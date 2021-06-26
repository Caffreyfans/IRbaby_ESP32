/*
 * @Author: Caffreyfans
 * @Date: 2021-06-18 00:33:23
 * @LastEditTime: 2021-06-26 23:53:41
 * @Description:
 */
/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>

#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "irbaby.h"
#include "lwip/dns.h"
#include "lwip/err.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"
#include "web.h"
#include "wifimanager.h"
#include "handler.h"
static const char *TAG = "IRbaby";
static void http_get_task(void *params) {
  (void)params;
  t_remote_ac_status status = {
      .ac_power = AC_POWER_ON,
      .ac_temp = AC_TEMP_16,
      .ac_mode = AC_MODE_COOL,
      .ac_wind_dir = AC_SWING_ON,
      .ac_wind_speed = AC_WS_AUTO,
  };
  irbaby_send("new_ac_2582", &status);
  vTaskDelete(NULL);
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
  wifi_init();
}
