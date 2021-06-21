/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 17:51:39
 * @LastEditTime: 2021-06-22 00:31:09
 * @Description:
 */
#include "handler.h"

#include "esp_log.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "esp_wifi.h"
#include "wifimanager.h"
#include "esp_spi_flash.h"
#define SCAN_LIST_SIZE 10

static const char *TAG = "handler";

char *connect_wifi(const char *ssid, const char *pass) { return NULL; }

char *scan_wifi_handle() {
  char *response = NULL;
  cJSON *root = cJSON_CreateArray();
  if (root == NULL) return NULL;
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

char *get_system_info_handle() {
  cJSON *root = cJSON_CreateObject();
  if (root == NULL) return NULL;

  time_t now;
  char strftime_buf[64];
  struct tm timeinfo;

  time(&now);
  // Set timezone to China Standard Time
  setenv("TZ", "CST-8", 1);
  tzset();

  localtime_r(&now, &timeinfo);
  strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);

  ESP_LOGI(TAG, "boot:: %s", strftime_buf);
  ESP_LOGI(TAG, "Build time: %s", __DATE__);
  ESP_LOGI(TAG, "core_version: %s", esp_get_idf_version());
  ESP_LOGI(TAG, "cput_freq: %d", ets_get_cpu_frequency());
  ESP_LOGI(TAG, "flash_size: %d", spi_flash_get_chip_size());
  // ESP_LOGI(TAG, "flash_speed: %s", );
  // ESP_LOGI(TAG, "free_heap: %s", );
  // ESP_LOGI(TAG, "free_sketch_space: %s", );
  size_t total_bytes, used_bytes;
  esp_spiffs_info(NULL, &total_bytes, &used_bytes);
  ESP_LOGI(TAG, "fs_total: %d", total_bytes);
  ESP_LOGI(TAG, "fs_used: %d", used_bytes);
  esp_reset_reason_t reset_reason = esp_reset_reason();
  ESP_LOGI(TAG, "reset_reason: %d", reset_reason);
  // ESP_LOGI(TAG, "sdk_version: %s", );
  esp_netif_ip_info_t ip_info;
  esp_netif_get_ip_info(IP_EVENT_STA_GOT_IP, &ip_info);
  ESP_LOGI(TAG, "wifi_ip: "IPSTR"" , IP2STR(&ip_info.ip));
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  ESP_LOGI(TAG, "wifi_mac:"MACSTR"", MAC2STR(mac));
  wifi_ap_record_t ap;
  esp_wifi_sta_get_ap_info(&ap);
  ESP_LOGI(TAG, "wifi_rssi: %d", ap.rssi);
  ESP_LOGI(TAG, "wifi_ssid: %s", (char *)ap.ssid);
  return NULL;
}