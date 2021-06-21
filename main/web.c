/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 15:57:08
 * @LastEditTime: 2021-06-20 23:11:06
 * @Description:
 */
#include "web.h"

#include <esp_err.h>
#include <esp_event.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <string.h>

#include "form_parser.h"
#include "handler.h"
#include "sys/param.h"

static const char *TAG = "WEB";
extern const char wificonfig_html[] asm("_binary_wificonfig_html_start");
extern const char root_html[] asm("_binary_root_html_start");
extern const char config_json[] asm("_binary_config_json_start");

static esp_err_t index_handler(httpd_req_t *req) {
  httpd_resp_sendstr(req, (const char *)req->user_ctx);
  return ESP_OK;
}
static esp_err_t root_handler(httpd_req_t *req) {
  httpd_resp_sendstr(req, (const char *)req->user_ctx);
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
  if (total_len >= 1024) {
    /* Respond with 500 Internal Server Error */
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR,
                        "content too long");
    return ESP_FAIL;
  }
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
  char *response = scan_wifi_handle();
  if (response != NULL) {
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, response);
  } else {
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "memory run out");
  }
  return ESP_OK;
}

httpd_handle_t start_webserver(void) {
  httpd_handle_t server = NULL;
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.lru_purge_enable = true;
  char *buffer = (char *)calloc(1024, sizeof(char));
  const httpd_uri_t wificonfig_post = {.uri = "/wificonfig",
                                       .method = HTTP_POST,
                                       .handler = wificonfig_handler,
                                       .user_ctx = buffer};

  const httpd_uri_t wificonfig_get = {.uri = "/wificonfig",
                                      .method = HTTP_GET,
                                      .handler = wificonfig_handler,
                                      .user_ctx = (void *)wificonfig_html};

  const httpd_uri_t root = {.uri = "/",
                            .method = HTTP_GET,
                            .handler = root_handler,
                            .user_ctx = (void *)root_html};

  const httpd_uri_t index = {.uri = "/index",
                             .method = HTTP_GET,
                             .handler = index_handler,
                             .user_ctx = (void *)config_json};

  ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
  if (httpd_start(&server, &config) == ESP_OK) {
    // Set URL handlers
    ESP_LOGI(TAG, "Registering URL handlers");
    httpd_register_uri_handler(server, &wificonfig_post);
    httpd_register_uri_handler(server, &wificonfig_get);
    httpd_register_uri_handler(server, &root);
    httpd_register_uri_handler(server, &index);
  }
  return server;
}
