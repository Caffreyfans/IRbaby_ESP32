/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 17:51:39
 * @LastEditTime: 2021-06-20 21:32:11
 * @Description:
 */
#include "handler.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "wifimanager.h"
#define SCAN_LIST_SIZE 10

char *connect_wifi(const char *ssid, const char *pass) { return NULL; }

char *scan_wifi_handle() {
  char *response = NULL;
  cJSON *root = cJSON_CreateArray();
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