/*
 * @Author: Caffreyfans
 * @Date: 2021-06-18 00:33:23
 * @LastEditTime: 2021-06-18 00:35:41
 * @Description: 
 */
/* HTTP GET Example using plain POSIX sockets

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"
#include "irbaby.h"
static void http_get_task(void *params)
{
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
void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(&http_get_task, "http_get_task", 4096, NULL, 5, NULL);
}
