/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 17:51:39
 * @LastEditTime: 2021-06-26 23:52:02
 * @Description:
 */
#include "web_handler.h"

#include "cJSON.h"
#include "conf.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "irext_api.h"
#include "version.h"
#include "wifimanager.h"
#include "esp_mac.h"
#include "esp_spi_flash.h"
#include "esp_psram.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "peripherals.h"
#ifdef CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32C2
#include "esp32c2/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32C5
#include "esp32c5/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32C6
#include "esp32c6/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32C61
#include "esp32c61/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32H2
#include "esp32h2/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32P4
#include "esp32p4/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/ets_sys.h"
#endif
#ifdef CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/ets_sys.h"
#endif
#define SCAN_LIST_SIZE 10

#define TAG "handler.c"
static const char *APP_KEY = "00567968d9316665fe5180fa2b1a24aa";
static const char *APP_SECRET = "6a111f71549eb485e616252f208017f4";
#define CHECK_IS_NULL(value, ret) \
  if (value == NULL)              \
    return ret;

static char *get_conf_handle(conf_type type)
{
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  if (root == NULL)
    return NULL;
  property_t *property = irbaby_get_conf(type);
  for (int i = 0; i < irbaby_get_conf_num(type); i++)
  {
    if (type == CONF_AC)
    {

      cJSON_AddNumberToObject(root, irbaby_get_conf_label(type, i), property[i].value);
    }
    else if (type == CONF_PIN)
    {
      cJSON_AddNumberToObject(root, irbaby_get_conf_label(type, i), property[i].value);
    }
  }
  response = cJSON_Print(root);
  cJSON_Delete(root);
  return response;
}

char *connect_wifi_handle(const char *ssid, const char *pass)
{
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  if (root == NULL)
    goto exit;
  if (wifi_connect_to_ap(ssid, pass) == true)
  {
    cJSON_AddStringToObject(root, "code", "0");
    cJSON_AddStringToObject(root, "msg", "SSID and PASSWORD are saved.");
  }
  else
  {
    cJSON_AddStringToObject(root, "code", "-1");
    cJSON_AddStringToObject(root, "msg", "can not connect to ap");
  }
  response = cJSON_Print(root);
exit:
  if (root != NULL)
    cJSON_Delete(root);
  return response;
}

char *scan_wifi_handle()
{
  char *response = NULL;
  cJSON *root = cJSON_CreateArray();
  if (root == NULL)
    goto exit;
  wifi_ap_record_t ap_infos[SCAN_LIST_SIZE];
  uint8_t ap_count = wifi_scan_ap(ap_infos, SCAN_LIST_SIZE);
  if (ap_count == 0)
    goto exit;
  cJSON *item = NULL;
  for (int i = 0; (i < SCAN_LIST_SIZE && i < ap_count); i++)
  {
    item = cJSON_CreateObject();
    cJSON_AddNumberToObject(item, "rssi", ap_infos[i].rssi);
    cJSON_AddStringToObject(item, "ssid", (const char *)ap_infos[i].ssid);
    cJSON_AddNumberToObject(item, "channel", ap_infos[i].primary);
    cJSON_AddItemToArray(root, item);
  }
  response = cJSON_Print(root);
exit:
  if (root != NULL)
    cJSON_Delete(root);
  return response;
}

char *get_info_handle()
{
  char *response = NULL;
  uint32_t flash_size;
  cJSON *root = cJSON_CreateObject();
  if (root == NULL)
    return NULL;

#define RET_BUFFER_SIZE 128
#define TMP_BUFFER_SIZE 64
  char ret_buffer[RET_BUFFER_SIZE];
  char tmp_buffer[TMP_BUFFER_SIZE];

  int64_t tick = xTaskGetTickCount();
  snprintf(ret_buffer, RET_BUFFER_SIZE, "%ldS", pdTICKS_TO_MS(tick) / 1000);
  cJSON_AddStringToObject(root, "boot", ret_buffer);

  itoa(heap_caps_get_total_size(MALLOC_CAP_32BIT) / 1024, tmp_buffer, 10);
  snprintf(ret_buffer, RET_BUFFER_SIZE, "%sKB", tmp_buffer);
  cJSON_AddStringToObject(root, "total_heap", ret_buffer);

  itoa(heap_caps_get_free_size(MALLOC_CAP_32BIT) / 1024, tmp_buffer, 10);
  snprintf(ret_buffer, RET_BUFFER_SIZE, "%sKB", tmp_buffer);
  cJSON_AddStringToObject(root, "free_heap", ret_buffer);

  static const char *reset_reason_str[] = {"can not be determined",
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
  esp_read_mac(mac, ESP_IF_WIFI_STA);
  snprintf(ret_buffer, RET_BUFFER_SIZE, MACSTR, MAC2STR(mac));
  cJSON_AddStringToObject(root, "wifi_mac", ret_buffer);

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%d", ap.rssi);
  cJSON_AddStringToObject(root, "wifi_rssi", ret_buffer);

  cJSON_AddStringToObject(root, "build_time", __DATE__);

  // cJSON_AddStringToObject(root, "sketch_size", );

  // cJSON_AddStringToObject(root, "free_sketch_size", );

  cJSON_AddStringToObject(root, "core_version", CORE_VERSION);

  cJSON_AddStringToObject(root, "sdk_version", esp_get_idf_version());

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%ldMHz", ets_get_cpu_frequency());
  cJSON_AddStringToObject(root, "cpu_freq", ret_buffer);
  esp_flash_get_size(NULL, &flash_size);
  snprintf(ret_buffer, RET_BUFFER_SIZE, "%ldMB", flash_size / 1024 / 1024);
  cJSON_AddStringToObject(root, "flash_size", ret_buffer);

  // cJSON_AddStringToObject(root, "flash_speed", );
  size_t total_bytes, used_bytes;
  esp_spiffs_info("spiffs", &total_bytes, &used_bytes);

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%dKB", total_bytes / 1024 / 1024);
  cJSON_AddStringToObject(root, "fs_total", ret_buffer);

  snprintf(ret_buffer, RET_BUFFER_SIZE, "%dKB", used_bytes / 1024 / 1024);
  cJSON_AddStringToObject(root, "fs_used", ret_buffer);
  response = cJSON_Print(root);
  cJSON_Delete(root);
  return response;
}

char *get_index_handle()
{
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

  cJSON *token_obj = irext_login(APP_KEY, APP_SECRET, "2");
  char *token_str = cJSON_GetObjectItem(token_obj, "token")->valuestring;
  int id = cJSON_GetObjectItem(token_obj, "id")->valueint;
  property_t *property = irbaby_get_conf(CONF_AC);
  const int brand_id = property[CONF_AC_BRAND].value;
  cJSON *indexes_obj = irext_list_indexes(1, brand_id, id, token_str);
  cJSON_ReplaceItemInObject(cell2, "options", indexes_obj);
  response = cJSON_PrintUnformatted(root);
  return response;
}

static char *get_protocol_handle(int brand_id)
{
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  cJSON *token_obj = irext_login(APP_KEY, APP_SECRET, "2");
  char *token_str = cJSON_GetObjectItem(token_obj, "token")->valuestring;
  int id = cJSON_GetObjectItem(token_obj, "id")->valueint;
  cJSON *indexes_obj = irext_list_indexes(1, brand_id, id, token_str);

  if (indexes_obj != NULL)
  {
    // update protocol
    cJSON *item = cJSON_GetArrayItem(indexes_obj, 0);
    irbaby_set_conf(CONF_AC, CONF_AC_PROTOCOL,
                    cJSON_GetObjectItem(item, "value")->valueint);
    cJSON_AddItemToObject(root, "__protocol__", indexes_obj);
    property_t *property = irbaby_get_conf(CONF_AC);
    for (int i = 0; i < irbaby_get_conf_num(CONF_AC); i++)
    {
      cJSON_AddNumberToObject(root, property[i].label, property[i].value);
    }
    response = cJSON_PrintUnformatted(root);
  }
  return response;
}

char *get_ir_handle() { return get_conf_handle(CONF_AC); }

char *get_gpio_handle() { return get_conf_handle(CONF_PIN); }

char *get_more_handle()
{
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  if (root != NULL)
  {
    response = cJSON_PrintUnformatted(root);
  }
  return response;
}

char *set_ir_handle(ir_ops ops, int value)
{
  irbaby_set_conf(CONF_AC, ops, value);
  switch (ops)
  {
  case SET_AC_BRAND:
    return get_protocol_handle(value);
    break;

  default:
    return get_ir_handle();
    break;
  }
}

char *set_gpio_handle(pin_ops ops, int value)
{
  irbaby_set_conf(CONF_PIN, ops, value);
  property_t *ac_pin = irbaby_get_conf(CONF_PIN);
  int tx_pin = ac_pin[CONF_PIN_IR_SEND].value;
  int rx_pin = ac_pin[CONF_PIN_IR_RECV].value;
  ir_init(tx_pin, rx_pin);
  return get_gpio_handle();
}