/*
 * @Author: Caffreyfans
 * @Date: 2021-06-20 14:16:30
 * @LastEditTime: 2021-06-29 23:35:38
 * @Description:
 */
#include "wifimanager.h"

#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <lwip/dns.h>
#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <string.h>
#include "dns_server.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_system.h"
#include "web.h"
#define AP_CHANNEL 1
#define MAX_STATION_CONNECTION 2
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1
#define WIFI_CONFIG_PATH "wifi"

static EventGroupHandle_t s_wifi_event_group;
esp_netif_t *g_station_netif = NULL;
wifi_config_t sta_config = {
    .sta = {.threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {.capable = true, .required = false}},
};
static esp_err_t wifi_event_handler(void *user_parameter,
                                    system_event_t *event) {
  switch (event->event_id) {
    case SYSTEM_EVENT_STA_GOT_IP:
      dns_server_stop();
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      esp_wifi_set_mode(WIFI_MODE_STA);
      start_webserver();
      break;
    case SYSTEM_EVENT_AP_START:
      dns_server_start();
      start_webserver();
      break;
    default:
      break;
  }
  return ESP_OK;
}

static char *get_chip_id() {
  const uint8_t mac_str_len = 7;
  char *mac_str = (char *)calloc(sizeof(char), mac_str_len);
  if (mac_str == NULL) return NULL;
  uint8_t mac[6] = {0};
  // Get MAC address for WiFi Station interface
  ESP_ERROR_CHECK(esp_read_mac(mac, ESP_MAC_WIFI_STA));
  snprintf(mac_str, mac_str_len, "%02X%02X%02X", mac[0], mac[2], mac[4]);
  mac_str[mac_str_len] = 0x00;
  return mac_str;
}

bool wifi_start_ap() {
  char *chip_id = get_chip_id();
  char ssid[52];
  snprintf(ssid, 52, "IRbaby_%s", chip_id);
  free(chip_id);
  wifi_config_t wifi_config = {
      .ap = {.ssid_len = strlen(ssid),
             .channel = 1,
             .password = "",
             .max_connection = 2,
             .authmode = WIFI_AUTH_OPEN},
  };
  strcpy((char *)wifi_config.ap.ssid, ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
  return true;
}

bool wifi_start_station(const char *ssid, const char *pass) {
  strcpy((char *)sta_config.sta.ssid, ssid);
  strcpy((char *)sta_config.sta.password, pass);
  esp_wifi_set_config(WIFI_IF_STA, &sta_config);
  esp_wifi_connect();
  /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or
   * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT). The
   * bits are set by event_handler() (see above) */
  EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT,
                                         pdFALSE, pdFALSE, pdMS_TO_TICKS(2000));

  /* The event will not be processed after unregister */
  /* xEventGroupWaitBits() returns the bits before the call returned, hence we
   * can test which event actually happened. */
  if (bits & WIFI_CONNECTED_BIT) {
    esp_wifi_set_storage(WIFI_STORAGE_FLASH);
    return true;
  } else {
    return false;
  }
}

uint8_t wifi_scan_ap(wifi_ap_record_t *ap_infos, const uint8_t size) {
  uint16_t number = size;
  uint16_t ap_count = 0;
  if (ap_infos == NULL || size == 0) {
    return ap_count;
  }

  esp_wifi_scan_start(NULL, true);
  esp_wifi_scan_get_ap_records(&number, ap_infos);
  esp_wifi_scan_get_ap_num(&ap_count);
  return ap_count;
}

void wifi_init() {
  s_wifi_event_group = xEventGroupCreate();
  esp_netif_init();
  esp_event_loop_init(wifi_event_handler, NULL);
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_netif_create_default_wifi_ap();
  g_station_netif = esp_netif_create_default_wifi_sta();
  esp_wifi_start();
  wifi_config_t sta_config;
  if (esp_wifi_get_config(WIFI_IF_STA, &sta_config) == ESP_OK) {
    esp_wifi_set_config(WIFI_IF_STA, &sta_config);
    // if (esp_wifi_connect() != ESP_OK) {
      wifi_start_ap();
    // }
  } else {
    wifi_start_ap();
  }
}
