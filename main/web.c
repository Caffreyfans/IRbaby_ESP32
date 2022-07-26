/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 15:57:08
 * @LastEditTime: 2021-06-29 23:45:24
 * @Description:
 */
#include "web.h"

#include "conf.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "form_parser.h"
#include "handler.h"
#include "nvs_flash.h"
#include "string.h"
#include "sys/param.h"

static const char *TAG = "web.c";

extern const char wificonfig_html[] asm("_binary_wificonfig_html_start");
extern const char root_html[] asm("_binary_root_html_start");
extern const char config_json[] asm("_binary_config_json_start");
extern const char irext_js[] asm("_binary_irext_js_start");

static esp_err_t index_handler(httpd_req_t *req) {
  char *response = NULL;
#define BUFFER_SIZE 512
  char query_str[BUFFER_SIZE];
  char buffer[BUFFER_SIZE];
  httpd_req_get_url_query_str(req, query_str, BUFFER_SIZE);
  if (httpd_query_key_value(query_str, "page", buffer, BUFFER_SIZE) == ESP_OK) {
    response = get_index_handle();
  } else if (httpd_query_key_value(query_str, "sync", buffer, BUFFER_SIZE) ==
             ESP_OK) {
    int tab = atoi(buffer);
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
  } else {
    // ir
    if (httpd_query_key_value(query_str, "brand", buffer, BUFFER_SIZE) ==
        ESP_OK) {
      response = get_protocol_handle(atoi(buffer));
    } else if (httpd_query_key_value(query_str, "protocol", buffer,
                                     BUFFER_SIZE) == ESP_OK) {
      response = set_ir_handle(CONF_AC_PROTOCOL, atoi(buffer));
    } else if (httpd_query_key_value(query_str, "power", buffer, BUFFER_SIZE) ==
               ESP_OK) {
      response = set_ir_handle(CONF_AC_POWER, atoi(buffer));
    } else if (httpd_query_key_value(query_str, "mode", buffer, BUFFER_SIZE) ==
               ESP_OK) {
      response = set_ir_handle(CONF_AC_MODE, atoi(buffer));
    } else if (httpd_query_key_value(query_str, "temperature", buffer,
                                     BUFFER_SIZE) == ESP_OK) {
      response = set_ir_handle(CONF_AC_TEMPERATURE, atoi(buffer));
    } else if (httpd_query_key_value(query_str, "fan", buffer, BUFFER_SIZE) ==
               ESP_OK) {
      response = set_ir_handle(CONF_AC_FAN, atoi(buffer));
    } else if (httpd_query_key_value(query_str, "fan_speed", buffer,
                                     BUFFER_SIZE) == ESP_OK) {
      response = set_ir_handle(CONF_AC_FAN_SPEED, atoi(buffer));
    }
    if (httpd_query_key_value(query_str, "fan_direction", buffer,
                              BUFFER_SIZE) == ESP_OK) {
      response = set_ir_handle(CONF_AC_FAN_DIRECVTION, atoi(buffer));
    }
    if (httpd_query_key_value(query_str, "pin_ir_send", buffer, BUFFER_SIZE) ==
        ESP_OK) {
      response = set_gpio_handle(CONF_PIN_IR_SEND, atoi(buffer));
    }
    if (httpd_query_key_value(query_str, "pin_ir_recv", buffer, BUFFER_SIZE) ==
        ESP_OK) {
      response = set_gpio_handle(CONF_PIN_IR_RECV, atoi(buffer));
    }
    if (httpd_query_key_value(query_str, "pin_led", buffer, BUFFER_SIZE) ==
        ESP_OK) {
      response = set_gpio_handle(CONF_PIN_LED, atoi(buffer));
    }
    if (httpd_query_key_value(query_str, "pin_button", buffer, BUFFER_SIZE) ==
        ESP_OK) {
      response = set_gpio_handle(CONF_PIN_BUTTON, atoi(buffer));
    }
  }
  if (response != NULL) {
    httpd_resp_sendstr(req, response);
    free(response);
  } else {
    httpd_resp_sendstr(req, "");
  }
  return ESP_OK;
}
static esp_err_t root_handler(httpd_req_t *req) {
  wifi_ap_record_t record;
  if (esp_wifi_sta_get_ap_info(&record) == ESP_OK) {
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

  // connect to wifi
  int total_len = req->content_len;
  int cur_len = 0;
  int received = 0;
  char buf[256];
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

static httpd_handle_t server = NULL;

void start_webserver(void) {
  if (server != NULL) {
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

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);

  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URL handlers
    httpd_register_uri_handler(server, &wificonfig_post);
    httpd_register_uri_handler(server, &wificonfig_get);
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &generate_204);
    httpd_register_uri_handler(server, &index_get);
  }
}

void shutdown_webserver(void) {
  if (server != NULL) {
    ESP_LOGI(TAG, "shutdown web server");
    httpd_stop(server);
    server = NULL;
  }
}