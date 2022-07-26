/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 17:51:39
 * @LastEditTime: 2021-06-26 23:52:02
 * @Description:
 */
#include "handler.h"

#include "cJSON.h"
#include "conf.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spi_flash.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "irext_api.h"
#include "version.h"
#include "wifimanager.h"
#define SCAN_LIST_SIZE 10

static const char *TAG = "handler.c";

#define CHECK_IS_NULL(value, ret) \
  if (value == NULL) return ret;

static char *get_conf_handle(conf_type type) {
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  if (root == NULL) return NULL;
  property_t *property = irbaby_get_conf(type);
  for (int i = 0; i < irbaby_get_conf_num(type); i++) {
    cJSON_AddNumberToObject(root, property[i].key, property[i].value);
  }
  response = cJSON_Print(root);
  cJSON_Delete(root);
  return response;
}

char *connect_wifi_handle(const char *ssid, const char *pass) {
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  if (root == NULL) goto exit;
  if (wifi_connect_to_ap(ssid, pass) == true) {
    cJSON_AddStringToObject(root, "code", "0");
    cJSON_AddStringToObject(root, "msg", "SSID and PASSWORD are saved.");

  } else {
    cJSON_AddStringToObject(root, "code", "-1");
    cJSON_AddStringToObject(root, "msg", "can not connect to ap");
  }
  response = cJSON_Print(root);
exit:
  if (root != NULL) cJSON_Delete(root);
  return response;
}

char *scan_wifi_handle() {
  char *response = NULL;
  cJSON *root = cJSON_CreateArray();
  if (root == NULL) goto exit;
  wifi_ap_record_t ap_infos[SCAN_LIST_SIZE];
  uint8_t ap_count = wifi_scan_ap(ap_infos, SCAN_LIST_SIZE);
  if (ap_count == 0) goto exit;
  cJSON *item = NULL;
  for (int i = 0; (i < SCAN_LIST_SIZE && i < ap_count); i++) {
    item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "rssi", ap_infos[i].rssi);
    cJSON_AddStringToObject(item, "ssid", (const char *)ap_infos[i].ssid);
    cJSON_AddNumberToObject(item, "channel", ap_infos[i].primary);
    cJSON_AddItemToArray(root, item);
  }
  response = cJSON_Print(root);
exit:
  if (root != NULL) cJSON_Delete(root);
  return response;
}

char *get_info_handle() {
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  if (root == NULL) return NULL;

#define RET_BUFFER_SIZE 128
#define TMP_BUFFER_SIZE 64
  char ret_buffer[RET_BUFFER_SIZE];
  char tmp_buffer[TMP_BUFFER_SIZE];

  int64_t tick = esp_timer_get_time();
  snprintf(ret_buffer, RET_BUFFER_SIZE, "%dS", pdTICKS_TO_MS(tick) / 1000);
  cJSON_AddStringToObject(root, "boot", ret_buffer);

  itoa(heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024, tmp_buffer, 10);
  snprintf(ret_buffer, RET_BUFFER_SIZE, "%sKB", tmp_buffer);
  cJSON_AddStringToObject(root, "free_heap", ret_buffer);

  const char *reset_reason_str[] = {"can not be determined",
                                    "power-on event",
                                    "external pin",
                                    "esp_restart",
                                    "exception/panic",
                                    "interrupt watchdog",
                                    "task watchdog",
                                    "other watchdogs",
                                    "exiting deep sleep mode",
                                    "browout reset",
                                    "SDIO"};
  cJSON_AddStringToObject(root, "reset_reason",
                          reset_reason_str[esp_reset_reason()]);

  wifi_ap_record_t ap;
  esp_wifi_sta_get_ap_info(&ap);
  cJSON_AddStringToObject(root, "wifi_ssid", (char *)ap.ssid);
  esp_netif_ip_info_t ip_info;
  esp_netif_get_ip_info(g_station_netif, &ip_info);
  snprintf(ret_buffer, RET_BUFFER_SIZE, IPSTR, IP2STR(&ip_info.ip));
  cJSON_AddStringToObject(root, "wifi_ip", ret_buffer);

  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(ret_buffer, RET_BUFFER_SIZE, MACSTR, MAC2STR(mac));
  cJSON_AddStringToObject(root, "wifi_mac", ret_buffer);

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%d", ap.rssi);
  cJSON_AddStringToObject(root, "wifi_rssi", ret_buffer);

  cJSON_AddStringToObject(root, "build_time", __DATE__);

  // cJSON_AddStringToObject(root, "sketch_size", );

  // cJSON_AddStringToObject(root, "free_sketch_size", );

  cJSON_AddStringToObject(root, "core_version", CORE_VERSION);

  cJSON_AddStringToObject(root, "sdk_version", esp_get_idf_version());

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%dMHz", ets_get_cpu_frequency());
  cJSON_AddStringToObject(root, "cpu_freq", ret_buffer);

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%dMB",
           spi_flash_get_chip_size() / 1024 / 1024);
  cJSON_AddStringToObject(root, "flash_size", ret_buffer);

  // cJSON_AddStringToObject(root, "flash_speed", );
  size_t total_bytes, used_bytes;
  esp_spiffs_info(NULL, &total_bytes, &used_bytes);

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%dKB", total_bytes / 1024);
  cJSON_AddStringToObject(root, "fs_total", ret_buffer);

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%dKB", used_bytes / 1024);
  cJSON_AddStringToObject(root, "fs_used", ret_buffer);
  response = cJSON_Print(root);
  cJSON_Delete(root);
  return response;
}

char *get_index_handle() {
  extern const char config_json[] asm("_binary_config_json_start");
  char *response = NULL;
  cJSON *root = cJSON_Parse(config_json);
  CHECK_IS_NULL(root, response);
  cJSON *tabs = cJSON_GetObjectItem(root, "tabs");
  CHECK_IS_NULL(tabs, response);
  cJSON *tab0 = cJSON_GetArrayItem(tabs, 0);
  CHECK_IS_NULL(tab0, response);
  cJSON *contents = cJSON_GetObjectItem(tab0, "contents");
  CHECK_IS_NULL(contents, response);
  cJSON *content0 = cJSON_GetArrayItem(contents, 0);
  CHECK_IS_NULL(content0, response);
  cJSON *cells = cJSON_GetObjectItem(content0, "cells");
  CHECK_IS_NULL(cells, response);
  cJSON *cell2 = cJSON_GetArrayItem(cells, 1);
  CHECK_IS_NULL(cell2, response);

  cJSON *token_obj = irext_login("cdf33048c9dbef2962b0f915bc7e420c",
                                 "f00f57af376c66ca1355cfe109400dd2", "2");
  char *token_str = cJSON_GetObjectItem(token_obj, "token")->valuestring;
  int id = cJSON_GetObjectItem(token_obj, "id")->valueint;
  property_t *property = irbaby_get_conf(CONF_AC);
  const int brand_id = property[CONF_AC_BRAND].value;
  cJSON *indexes_obj = irext_list_indexes(1, brand_id, id, token_str);
  cJSON_ReplaceItemInObject(cell2, "options", indexes_obj);
  response = cJSON_PrintUnformatted(root);
  return response;
}

char *get_protocol_handle(int brand_id) {
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  cJSON *token_obj = irext_login("cdf33048c9dbef2962b0f915bc7e420c",
                                 "f00f57af376c66ca1355cfe109400dd2", "2");
  char *token_str = cJSON_GetObjectItem(token_obj, "token")->valuestring;
  int id = cJSON_GetObjectItem(token_obj, "id")->valueint;
  cJSON *indexes_obj = irext_list_indexes(1, brand_id, id, token_str);

  if (indexes_obj != NULL) {
    // update protocol
    cJSON *item = cJSON_GetArrayItem(indexes_obj, 0);
    irbaby_set_conf(CONF_AC, CONF_AC_PROTOCOL,
                    cJSON_GetObjectItem(item, "value")->valueint);
    cJSON_AddItemToObject(root, "__protocol__", indexes_obj);
    property_t *property = irbaby_get_conf(CONF_AC);
    for (int i = 0; i < irbaby_get_conf_num(CONF_AC); i++) {
      cJSON_AddNumberToObject(root, property[i].key, property[i].value);
    }
    response = cJSON_PrintUnformatted(root);
  }
  return response;
}

char *get_ir_handle() { return get_conf_handle(CONF_AC); }

char *get_gpio_handle() { return get_conf_handle(CONF_PIN); }

char *get_more_handle() {
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  if (root != NULL) {
    response = cJSON_PrintUnformatted(root);
  }
  return response;
}

char *set_ir_handle(ac_ops ops, int value) {
  irbaby_set_conf(CONF_AC, ops, value);
  return get_ir_handle();
}

char *set_gpio_handle(pin_ops ops, int value) {
  irbaby_set_conf(CONF_PIN, ops, value);
  return get_gpio_handle();
}