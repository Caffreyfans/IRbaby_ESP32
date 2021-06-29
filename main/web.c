/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 15:57:08
 * @LastEditTime: 2021-06-29 23:45:24
 * @Description:
 */
#include "web.h"

#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "form_parser.h"
#include "handler.h"
#include "nvs_flash.h"
#include "string.h"
#include "sys/param.h"

static const char *TAG = "WEB";
extern const char wificonfig_html[] asm("_binary_wificonfig_html_start");
extern const char root_html[] asm("_binary_root_html_start");
extern const char config_json[] asm("_binary_config_json_start");
extern const char irext_js[] asm("_binary_irext_js_start");

static esp_err_t index_handler(httpd_req_t *req) {
  const int query_buffer_size = 512;
  const int value_buffer_size = 512;
  char query_str[value_buffer_size];
  char value[value_buffer_size];
  esp_err_t ret = ESP_FAIL;
  httpd_req_get_url_query_str(req, query_str, query_buffer_size);
  ret = httpd_query_key_value(query_str, "page", value, value_buffer_size);
  if (ret == ESP_OK) {
    httpd_resp_sendstr(req, (const char *)req->user_ctx);
    return ESP_OK;
  }
  char *response = NULL;
  ret = httpd_query_key_value(query_str, "sync", value, value_buffer_size);
  if (ret == ESP_OK) {
    int tab = atoi(value);
    switch (tab) {
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
      default:
        break;
    }
  }
  if (response != NULL) {
    httpd_resp_sendstr(req, response);
    free(response);
  }
  return ESP_OK;
}

static esp_err_t root_handler(httpd_req_t *req) {
  wifi_mode_t wifi_mode;
  esp_wifi_get_mode(&wifi_mode);
  if (wifi_mode == WIFI_MODE_STA) {
    httpd_resp_sendstr(req, (const char *)root_html);
  } else {
    httpd_resp_sendstr(req, (const char *)wificonfig_html);
  }
  return ESP_OK;
}

static esp_err_t wificonfig_handler(httpd_req_t *req) {
  if (req->method == HTTP_GET) {
    httpd_resp_sendstr(req, (const char *)req->user_ctx);
    return ESP_OK;
  }
  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;
  char *buf = (char *)req->user_ctx;
  while (cur_len < total_len) {
    received = httpd_req_recv(req, buf + cur_len, total_len);
    if (received <= 0) {
      /* Respond with 500 Internal Server Error */
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "Failed to post control value");
      return ESP_FAIL;
    }
    cur_len += received;
  }
  buf[total_len] = '\0';
  cJSON *root = form_parse(buf);
  if (root == NULL) return ESP_FAIL;
  cJSON *item = NULL;
  if ((item = cJSON_GetObjectItem(root, "scan")) != NULL) {
    char *response = scan_wifi_handle();
    if (response != NULL) {
      httpd_resp_set_type(req, HTTPD_TYPE_JSON);
      httpd_resp_sendstr(req, response);
      free(response);
    } else {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "memory run out");
    }
  } else if ((item = cJSON_GetObjectItem(root, "ssid")) != NULL) {
    char *ssid = item->valuestring;
    char *pass = cJSON_GetObjectItem(root, "pass")->valuestring;
    char *response = connect_wifi_handle(ssid, pass);
    if (response != NULL) {
      httpd_resp_sendstr(req, response);
      free(response);
    } else {
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                          "memory run out");
    }
  } else {
    ESP_LOGI(TAG, "can not get anything");
  }
  return ESP_OK;
}

static esp_err_t irext_handler(httpd_req_t *req) {
  httpd_resp_sendstr(req, (char *)req->user_ctx);
  return ESP_OK;
}

static httpd_handle_t server = NULL;
static char *g_user_ctx = NULL;
void start_webserver(void) {
  ESP_LOGI(TAG, "try to start web server");
  if (server != NULL) {
    shutdown_webserver();
  }
  ESP_LOGI(TAG, "web server started");
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;
  g_user_ctx = (char *)calloc(1024, sizeof(char));
  const httpd_uri_t wificonfig_post = {.uri = "/wificonfig",
                                       .method = HTTP_POST,
                                       .handler = wificonfig_handler,
                                       .user_ctx = g_user_ctx};

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
                                 .user_ctx = (void *)config_json};

  const httpd_uri_t irext = {.uri = "/irext.js",
                             .method = HTTP_GET,
                             .handler = irext_handler,
                             .user_ctx = (void *)irext_js};

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URL handlers
    ESP_LOGI(TAG, "Registering URL handlers");
    httpd_register_uri_handler(server, &wificonfig_post);
    httpd_register_uri_handler(server, &wificonfig_get);
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &generate_204);
    httpd_register_uri_handler(server, &index_get);
    httpd_register_uri_handler(server, &irext);
  }
}

void shutdown_webserver(void) {
  if (server != NULL) {
    ESP_LOGI(TAG, "shutdown web server");
    httpd_stop(server);
    if (g_user_ctx != NULL) {
      free(g_user_ctx);
      g_user_ctx = NULL;
    }
    server = NULL;
  }
}