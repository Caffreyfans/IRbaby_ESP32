/*
 * @Author: Caffreyfans
 * @Date: 2021-06-18 00:33:23
 * @LastEditTime: 2021-06-29 23:21:59
 * @Description:
 */
/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>

#include "cJSON.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "irbaby.h"
#include "irext_api.h"
#include "nvs_flash.h"
#include "wifimanager.h"
static const char *TAG = "IRbaby";

static void irbaby_start() {
  wifi_init();
  cJSON *token_obj = irext_login("cdf33048c9dbef2962b0f915bc7e420c",
                                 "f00f57af376c66ca1355cfe109400dd2", "2");
  char *token_str = cJSON_GetObjectItem(token_obj, "token")->valuestring;
  int id_num = cJSON_GetObjectItem(token_obj, "id")->valueint;
  cJSON *categories_obj = irext_list_categories(id_num, token_str);
  cJSON *brands_obj = irext_list_brands(1, id_num, token_str);
  cJSON *indexes_obj = irext_list_indexes(1, 1, id_num, token_str);

  char *str = cJSON_PrintUnformatted(categories_obj);
  printf("%s\n", str);
  free(str);
  str = cJSON_PrintUnformatted(brands_obj);
  printf("%s\n", str);
  free(str);
  str = cJSON_PrintUnformatted(indexes_obj);
  printf("%s\n", str);
  free(str);

  cJSON_Delete(categories_obj);
  cJSON_Delete(token_obj);
  cJSON_Delete(indexes_obj);
}

void app_main(void) {
  ESP_ERROR_CHECK(nvs_flash_init());
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  esp_vfs_spiffs_conf_t conf = {.base_path = "",
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = true};

  // Use settings defined above to initialize and mount SPIFFS filesystem.
  // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
  esp_vfs_spiffs_register(&conf);
  irbaby_start();
}
