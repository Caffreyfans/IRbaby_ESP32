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

#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "sdkconfig.h"
#include "irbaby.h"
#include "irext_api.h"
#include "nvs_flash.h"
#include "conf.h"
#include "ir.h"
#include "peripherals.h"
#if CONFIG_SOC_WIFI_SUPPORTED
#include "esp_wifi.h"
#include "wifimanager.h"
#include "web.h"
#include "mqtt.h"
#include "homekit.h"
#endif
static const char *TAG = "IRbaby";

void app_main(void)
{
  // Initialize NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_LOGI(TAG, "Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};

  // Use settings defined above to initialize and mount SPIFFS filesystem.
  // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
  ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK)
  {
    if (ret == ESP_FAIL)
    {
      ESP_LOGE(TAG, "Failed to mount or format filesystem");
    }
    else if (ret == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE(TAG, "Failed to find SPIFFS partition");
    }
    else
    {
      ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
    }
    return;
  }

  size_t total = 0, used = 0;
  ret = esp_spiffs_info(conf.partition_label, &total, &used);
  if (ret != ESP_OK)
  {
    ESP_LOGI(TAG,
             "Failed to get SPIFFS partition information (%s). Formatting...",
             esp_err_to_name(ret));
    esp_spiffs_format(conf.partition_label);
    return;
  }
  else
  {
    ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
  }
  irbaby_load_conf();
  property_t *pin_conf = irbaby_get_conf(CONF_PIN);
  int tx_pin = pin_conf[CONF_PIN_IR_SEND].value;
  int rx_pin = pin_conf[CONF_PIN_IR_RECV].value;
  ir_init(tx_pin, rx_pin);
#if CONFIG_SOC_WIFI_SUPPORTED
  wifi_init();
  start_webserver();
  
  // Initialize MQTT if enabled
  string_property_t *mqtt_conf = irbaby_get_string_conf(CONF_MQTT);
  if (strcmp(mqtt_conf[CONF_MQTT_ENABLE].value, "1") == 0) {
    esp_err_t mqtt_err = mqtt_init(mqtt_conf[CONF_MQTT_BROKER_URL].value,
                                     atoi(mqtt_conf[CONF_MQTT_BROKER_PORT].value),
                                     mqtt_conf[CONF_MQTT_USERNAME].value,
                                     mqtt_conf[CONF_MQTT_PASSWORD].value,
                                     mqtt_conf[CONF_MQTT_CLIENT_ID].value,
                                     mqtt_conf[CONF_MQTT_TOPIC_PREFIX].value);
    if (mqtt_err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to initialize MQTT: %s", esp_err_to_name(mqtt_err));
    }
  }

  // Initialize HomeKit
  homekit_init();
#else
  ESP_LOGW(TAG, "Wi-Fi is not supported on this target; web, MQTT, OTA, and HomeKit services are disabled.");
#endif
}
