/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 15:57:08
 * @LastEditTime: 2021-06-29 23:45:24
 * @Description:
 */
#include "web.h"

#include "conf.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "form_parser.h"
#include "ir.h"
#include "nvs_flash.h"
#include "string.h"
#include "sys/param.h"
#include "web_handler.h"
#include <stdio.h>
#include "esp_ota_ops.h"
static const char *TAG = "web.c";

extern const char wificonfig_html[] asm("_binary_wificonfig_html_start");
extern const char root_html[] asm("_binary_root_html_start");
extern const char config_json[] asm("_binary_config_json_start");
extern const char irext_js[] asm("_binary_irext_js_start");

typedef esp_err_t (*multipart_chunk_handler_t)(const uint8_t *data, size_t len, void *ctx);

static void *find_bytes(const void *haystack, size_t haystack_len,
                        const void *needle, size_t needle_len)
{
  if (needle_len == 0 || haystack_len < needle_len)
  {
    return NULL;
  }

  const uint8_t *bytes = (const uint8_t *)haystack;
  for (size_t i = 0; i <= haystack_len - needle_len; i++)
  {
    if (memcmp(bytes + i, needle, needle_len) == 0)
    {
      return (void *)(bytes + i);
    }
  }
  return NULL;
}

static bool is_safe_upload_filename(const char *filename)
{
  return filename != NULL && filename[0] != '\0' &&
         strchr(filename, '/') == NULL &&
         strchr(filename, '\\') == NULL &&
         strchr(filename, ':') == NULL &&
         strstr(filename, "..") == NULL;
}

static esp_err_t parse_multipart_boundary(httpd_req_t *req, char *boundary,
                                          size_t boundary_size)
{
  char content_type[128];
  if (httpd_req_get_hdr_value_str(req, "Content-Type", content_type,
                                  sizeof(content_type)) != ESP_OK)
  {
    return ESP_ERR_NOT_FOUND;
  }

  char *boundary_start = strstr(content_type, "boundary=");
  if (boundary_start == NULL)
  {
    return ESP_ERR_NOT_FOUND;
  }

  boundary_start += strlen("boundary=");
  if (*boundary_start == '"')
  {
    boundary_start++;
  }

  size_t len = 0;
  while (boundary_start[len] != '\0' && boundary_start[len] != ';' &&
         boundary_start[len] != '"' && len < boundary_size - 1)
  {
    boundary[len] = boundary_start[len];
    len++;
  }
  boundary[len] = '\0';
  return len == 0 ? ESP_ERR_NOT_FOUND : ESP_OK;
}

static esp_err_t extract_multipart_filename(const char *headers, char *filename,
                                            size_t filename_size)
{
  char *filename_start = strstr(headers, "filename=\"");
  if (filename_start == NULL)
  {
    return ESP_ERR_NOT_FOUND;
  }

  filename_start += strlen("filename=\"");
  char *filename_end = strchr(filename_start, '"');
  if (filename_end == NULL)
  {
    return ESP_ERR_NOT_FOUND;
  }

  size_t len = filename_end - filename_start;
  if (len == 0 || len >= filename_size)
  {
    return ESP_ERR_INVALID_SIZE;
  }

  memcpy(filename, filename_start, len);
  filename[len] = '\0';
  return is_safe_upload_filename(filename) ? ESP_OK : ESP_ERR_INVALID_ARG;
}

static esp_err_t process_multipart_payload(const uint8_t *data, size_t len,
                                           const char *marker,
                                           uint8_t *tail, size_t *tail_len,
                                           multipart_chunk_handler_t handler,
                                           void *ctx, bool *done)
{
  const size_t marker_len = strlen(marker);
  uint8_t combined[1200];

  if (*tail_len + len > sizeof(combined))
  {
    return ESP_ERR_INVALID_SIZE;
  }

  memcpy(combined, tail, *tail_len);
  memcpy(combined + *tail_len, data, len);
  size_t combined_len = *tail_len + len;

  uint8_t *marker_pos = find_bytes(combined, combined_len, marker, marker_len);
  if (marker_pos != NULL)
  {
    size_t payload_len = marker_pos - combined;
    if (payload_len > 0)
    {
      ESP_RETURN_ON_ERROR(handler(combined, payload_len, ctx), TAG,
                          "failed to write multipart payload");
    }
    *tail_len = 0;
    *done = true;
    return ESP_OK;
  }

  if (combined_len > marker_len)
  {
    size_t flush_len = combined_len - marker_len;
    ESP_RETURN_ON_ERROR(handler(combined, flush_len, ctx), TAG,
                        "failed to write multipart payload");
    memcpy(tail, combined + flush_len, marker_len);
    *tail_len = marker_len;
  }
  else
  {
    memcpy(tail, combined, combined_len);
    *tail_len = combined_len;
  }

  return ESP_OK;
}

static esp_err_t receive_multipart_file(httpd_req_t *req, char *filename,
                                        size_t filename_size,
                                        multipart_chunk_handler_t handler,
                                        void *ctx)
{
  char boundary[96];
  esp_err_t err = parse_multipart_boundary(req, boundary, sizeof(boundary));
  if (err != ESP_OK)
  {
    return err;
  }

  char marker[128];
  int marker_len = snprintf(marker, sizeof(marker), "\r\n--%s", boundary);
  if (marker_len <= 0 || marker_len >= sizeof(marker))
  {
    return ESP_ERR_INVALID_SIZE;
  }

  uint8_t recv_buf[1024];
  char header_buf[1024];
  size_t header_len = 0;
  uint8_t tail[128];
  size_t tail_len = 0;
  int remaining = req->content_len;
  bool headers_done = false;
  bool done = false;

  while (remaining > 0 && !done)
  {
    int received = httpd_req_recv(req, (char *)recv_buf,
                                  MIN(remaining, (int)sizeof(recv_buf)));
    if (received <= 0)
    {
      return ESP_FAIL;
    }
    remaining -= received;

    uint8_t *payload = recv_buf;
    size_t payload_len = received;
    if (!headers_done)
    {
      if (header_len + received > sizeof(header_buf))
      {
        return ESP_ERR_INVALID_SIZE;
      }

      memcpy(header_buf + header_len, recv_buf, received);
      header_len += received;
      char *header_end = find_bytes(header_buf, header_len, "\r\n\r\n", 4);
      if (header_end == NULL)
      {
        continue;
      }

      size_t header_size = header_end - header_buf + 4;
      header_buf[header_size - 2] = '\0';
      if (filename != NULL)
      {
        ESP_RETURN_ON_ERROR(extract_multipart_filename(header_buf, filename,
                                                       filename_size),
                            TAG, "invalid upload filename");
      }

      headers_done = true;
      payload = (uint8_t *)header_buf + header_size;
      payload_len = header_len - header_size;
    }

    if (payload_len > 0)
    {
      ESP_RETURN_ON_ERROR(process_multipart_payload(payload, payload_len, marker,
                                                   tail, &tail_len, handler,
                                                   ctx, &done),
                          TAG, "failed to process multipart payload");
    }
  }

  if (!headers_done || !done)
  {
    return ESP_ERR_INVALID_RESPONSE;
  }

  return ESP_OK;
}

static esp_err_t file_upload_chunk_handler(const uint8_t *data, size_t len,
                                           void *ctx)
{
  FILE *fd = (FILE *)ctx;
  return fwrite(data, 1, len, fd) == len ? ESP_OK : ESP_FAIL;
}

static esp_err_t ota_upload_chunk_handler(const uint8_t *data, size_t len,
                                          void *ctx)
{
  esp_ota_handle_t *ota_handle = (esp_ota_handle_t *)ctx;
  return esp_ota_write(*ota_handle, data, len);
}

static esp_err_t index_handler(httpd_req_t *req)
{
  char *response = NULL;
#define BUFFER_SIZE 512
  char query_str[BUFFER_SIZE];
  char buffer[BUFFER_SIZE];
  httpd_req_get_url_query_str(req, query_str, BUFFER_SIZE);
  if (httpd_query_key_value(query_str, "page", buffer, BUFFER_SIZE) == ESP_OK)
  {
    response = get_index_handle();
  }
  else if (httpd_query_key_value(query_str, "sync", buffer, BUFFER_SIZE) ==
           ESP_OK)
  {
    int tab = atoi(buffer);
    switch (tab)
    {
    case 0:
      response = get_ir_handle();
      break;
    case 1:
      response = get_gpio_handle();
      break;
    case 2:
      response = get_info_handle();
      break;
    case 3:
      response = get_more_handle();
      break;
    case 4:
      response = get_mqtt_handle();
      break;
    default:
      break;
    }
  }
  else
  {
    // ir
    if (httpd_query_key_value(query_str, "brand", buffer, BUFFER_SIZE) ==
        ESP_OK)
    {

      response = set_ir_handle(SET_AC_BRAND, (atoi(buffer)));
    }
    else if (httpd_query_key_value(query_str, "protocol", buffer,
                                   BUFFER_SIZE) == ESP_OK)
    {

      response = set_ir_handle(SET_AC_PROTOCOL, atoi(buffer));
    }
    else if (httpd_query_key_value(query_str, "power", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_ir_handle(SET_AC_POWER, atoi(buffer));
      ir_transmission(IR_SEND_DECODE);
    }
    else if (httpd_query_key_value(query_str, "mode", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_ir_handle(SET_AC_MODE, atoi(buffer));

      ir_transmission(IR_SEND_DECODE);
    }
    else if (httpd_query_key_value(query_str, "temperature", buffer,
                                   BUFFER_SIZE) == ESP_OK)
    {

      response = set_ir_handle(SET_AC_TEMPERATURE, atoi(buffer));

      ir_transmission(IR_SEND_DECODE);
    }
    else if (httpd_query_key_value(query_str, "fan", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_ir_handle(SET_AC_FAN, atoi(buffer));

      ir_transmission(IR_SEND_DECODE);
    }
    else if (httpd_query_key_value(query_str, "fan_speed", buffer,
                                   BUFFER_SIZE) == ESP_OK)
    {

      response = set_ir_handle(SET_AC_FAN_SPEED, atoi(buffer));

      ir_transmission(IR_SEND_DECODE);
    }
    else if (httpd_query_key_value(query_str, "fan_direction", buffer,
                                   BUFFER_SIZE) == ESP_OK)
    {

      response = set_ir_handle(SET_AC_FAN_DIRECVTION, atoi(buffer));

      ir_transmission(IR_SEND_DECODE);
    }
    else if (httpd_query_key_value(query_str, "ir_receive_enable", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_ir_handle(SET_IR_RECEIVE_ENABLE, atoi(buffer));
    }
    else if (httpd_query_key_value(query_str, "ir_send_incoming", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      ir_transmission(IR_SEND_INCOMING);
    }
    else if (httpd_query_key_value(query_str, "pin_ir_send", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_gpio_handle(CONF_PIN_IR_SEND, atoi(buffer));
    }
    else if (httpd_query_key_value(query_str, "pin_ir_recv", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_gpio_handle(CONF_PIN_IR_RECV, atoi(buffer));
    }
    else if (httpd_query_key_value(query_str, "pin_led", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_gpio_handle(CONF_PIN_LED, atoi(buffer));
    }
    else if (httpd_query_key_value(query_str, "pin_button", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_gpio_handle(CONF_PIN_BUTTON, atoi(buffer));
    }
    else if (httpd_query_key_value(query_str, "mqtt_enable", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_mqtt_handle(CONF_MQTT_ENABLE, buffer);
    }
    else if (httpd_query_key_value(query_str, "mqtt_broker_url", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_mqtt_handle(CONF_MQTT_BROKER_URL, buffer);
    }
    else if (httpd_query_key_value(query_str, "mqtt_broker_port", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_mqtt_handle(CONF_MQTT_BROKER_PORT, buffer);
    }
    else if (httpd_query_key_value(query_str, "mqtt_username", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_mqtt_handle(CONF_MQTT_USERNAME, buffer);
    }
    else if (httpd_query_key_value(query_str, "mqtt_password", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_mqtt_handle(CONF_MQTT_PASSWORD, buffer);
    }
    else if (httpd_query_key_value(query_str, "mqtt_client_id", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_mqtt_handle(CONF_MQTT_CLIENT_ID, buffer);
    }
    else if (httpd_query_key_value(query_str, "mqtt_topic_prefix", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {

      response = set_mqtt_handle(CONF_MQTT_TOPIC_PREFIX, buffer);
    }
    else if (httpd_query_key_value(query_str, "ota", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {
      response = ota_update_handle(buffer);
    }
    else if (httpd_query_key_value(query_str, "reboot", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {
      response = reboot_handle();
    }
    else if (httpd_query_key_value(query_str, "file_list", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {
      response = get_file_list_handle();
    }
    else if (httpd_query_key_value(query_str, "delete_file", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {
      response = delete_file_handle(buffer);
    }
    else if (httpd_query_key_value(query_str, "get_homekit", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {
      response = get_homekit_handle();
    }
    else if (httpd_query_key_value(query_str, "set_homekit_enable", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {
      response = set_homekit_handle(CONF_HOMEKIT_ENABLE, buffer);
    }
    else if (httpd_query_key_value(query_str, "set_homekit_setup_code", buffer, BUFFER_SIZE) ==
             ESP_OK)
    {
      response = set_homekit_handle(CONF_HOMEKIT_SETUP_CODE, buffer);
    }
  }
  if (response != NULL)
  {
    httpd_resp_sendstr(req, response);
    free(response);
  }
  else
  {
    httpd_resp_sendstr(req, "");
  }
  return ESP_OK;
}
static esp_err_t root_handler(httpd_req_t *req)
{
  wifi_ap_record_t record;
  if (esp_wifi_sta_get_ap_info(&record) == ESP_OK)
  {
    httpd_resp_sendstr(req, (const char *)root_html);
  }
  else
  {
    httpd_resp_sendstr(req, (const char *)wificonfig_html);
  }
  return ESP_OK;
}
static esp_err_t upload_handler(httpd_req_t *req)
{
  FILE *fd = NULL;
  char filename[256] = {0};
  char filepath[512];
  const char *tmp_filepath = "/spiffs/.upload.tmp";

  fd = fopen(tmp_filepath, "wb");
  if (fd == NULL)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
    return ESP_FAIL;
  }

  esp_err_t err = receive_multipart_file(req, filename, sizeof(filename),
                                         file_upload_chunk_handler, fd);
  fclose(fd);
  if (err != ESP_OK)
  {
    remove(tmp_filepath);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid upload request");
    return ESP_FAIL;
  }

  int path_len = snprintf(filepath, sizeof(filepath), "/spiffs/%s", filename);
  if (path_len < 0 || path_len >= sizeof(filepath))
  {
    remove(tmp_filepath);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Filename too long");
    return ESP_FAIL;
  }

  remove(filepath);
  if (rename(tmp_filepath, filepath) != 0)
  {
    remove(tmp_filepath);
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Upload failed");
    return ESP_FAIL;
  }

  httpd_resp_sendstr(req, "{\"status\":\"success\"}");
  return ESP_OK;
}

static esp_err_t firmware_upload_handler(httpd_req_t *req)
{
  esp_ota_handle_t ota_handle = 0;
  const esp_partition_t *ota_partition = NULL;
  
  // Find next OTA partition
  ota_partition = esp_ota_get_next_update_partition(NULL);
  if (ota_partition == NULL)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "No OTA partition found");
    return ESP_FAIL;
  }
  
  // Begin OTA update
  esp_err_t err = esp_ota_begin(ota_partition, OTA_SIZE_UNKNOWN, &ota_handle);
  if (err != ESP_OK)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
    return ESP_FAIL;
  }

  err = receive_multipart_file(req, NULL, 0, ota_upload_chunk_handler, &ota_handle);
  if (err != ESP_OK)
  {
    esp_ota_abort(ota_handle);
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Firmware upload failed");
    return ESP_FAIL;
  }
  
  // End OTA update
  err = esp_ota_end(ota_handle);
  if (err != ESP_OK)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA end failed");
    return ESP_FAIL;
  }
  
  // Set boot partition
  err = esp_ota_set_boot_partition(ota_partition);
  if (err != ESP_OK)
  {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA set boot partition failed");
    return ESP_FAIL;
  }
  
  httpd_resp_sendstr(req, "{\"status\":\"success\",\"message\":\"Firmware uploaded. Reboot to apply.\"}");
  return ESP_OK;
}

static esp_err_t wificonfig_handler(httpd_req_t *req)
{
  if (req->method == HTTP_GET)
  {
    httpd_resp_sendstr(req, (const char *)req->user_ctx);
    return ESP_OK;
  }

  // connect to wifi
  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;
  char buf[256];
  if (total_len < 0 || total_len >= (int)sizeof(buf))
  {
    httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Request body too large");
    return ESP_FAIL;
  }

  while (cur_len < total_len)
  {
    int receive_len = MIN(total_len - cur_len, (int)sizeof(buf) - cur_len - 1);
    received = httpd_req_recv(req, buf + cur_len, receive_len);
    if (received <= 0)
    {
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to post control value");
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';

  cJSON *root = form_parse(buf);
  if (root == NULL)
    return ESP_FAIL;

  cJSON *item = NULL;
  if ((item = cJSON_GetObjectItem(root, "scan")) != NULL)
  {
    char *response = scan_wifi_handle();
    if (response != NULL)
    {
      httpd_resp_set_type(req, HTTPD_TYPE_JSON);
      httpd_resp_sendstr(req, response);
      free(response);
    }
    else
    {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "memory run out");
    }
  }
  else if ((item = cJSON_GetObjectItem(root, "ssid")) != NULL)
  {
    char *ssid = item->valuestring;
    cJSON *pass_item = cJSON_GetObjectItem(root, "pass");
    if (ssid == NULL || pass_item == NULL || pass_item->valuestring == NULL)
    {
      cJSON_Delete(root);
      httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing wifi credentials");
      return ESP_FAIL;
    }
    char *pass = pass_item->valuestring;
    char *response = connect_wifi_handle(ssid, pass);
    if (response != NULL)
    {
      httpd_resp_sendstr(req, response);
      free(response);
    }
    else
    {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "memory run out");
    }
  }
  else
  {
    ESP_LOGI(TAG, "can not get anything");
  }
  cJSON_Delete(root);
  return ESP_OK;
}

static httpd_handle_t server = NULL;

void start_webserver(void)
{
  if (server != NULL)
  {
    shutdown_webserver();
  }
  ESP_LOGI(TAG, "web server started");
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.stack_size = 10240;
  config.lru_purge_enable = true;
  const httpd_uri_t wificonfig_post = {.uri = "/wificonfig",
                                       .method = HTTP_POST,
                                       .handler = wificonfig_handler,
                                       .user_ctx = NULL};

  const httpd_uri_t wificonfig_get = {.uri = "/wificonfig",
                                      .method = HTTP_GET,
                                      .handler = wificonfig_handler,
                                      .user_ctx = (void *)wificonfig_html};

  const httpd_uri_t root = {.uri = "/",
                            .method = HTTP_GET,
                            .handler = root_handler,
                            .user_ctx = NULL};

  const httpd_uri_t generate_204 = {.uri = "/generate_204",
                                    .method = HTTP_GET,
                                    .handler = root_handler,
                                    .user_ctx = NULL};

  const httpd_uri_t index_get = {.uri = "/index",
                                 .method = HTTP_GET,
                                 .handler = index_handler,
                                 .user_ctx = NULL};

  const httpd_uri_t upload_post = {.uri = "/upload",
                                   .method = HTTP_POST,
                                   .handler = upload_handler,
                                   .user_ctx = NULL};

  const httpd_uri_t firmware_upload_post = {.uri = "/firmware_upload",
                                           .method = HTTP_POST,
                                           .handler = firmware_upload_handler,
                                           .user_ctx = NULL};

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

  if (httpd_start(&server, &config) == ESP_OK)
  {
    // Set URL handlers
    httpd_register_uri_handler(server, &wificonfig_post);
    httpd_register_uri_handler(server, &wificonfig_get);
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &generate_204);
    httpd_register_uri_handler(server, &index_get);
    httpd_register_uri_handler(server, &upload_post);
    httpd_register_uri_handler(server, &firmware_upload_post);
  }
}

void shutdown_webserver(void)
{
  if (server != NULL)
  {
    ESP_LOGI(TAG, "shutdown web server");
    httpd_stop(server);
    server = NULL;
  }
}
