/*
 * @Author: Caffreyfans
 * @Date: 2021-06-20 14:16:30
 * @LastEditTime: 2021-06-26 23:17:40
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
static const char *TAG = "WIFI";

static TaskHandle_t dns_server = NULL;
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
      xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
      esp_wifi_set_mode(WIFI_MODE_STA);
      start_webserver();
      break;
    case SYSTEM_EVENT_AP_START:
      start_webserver();
      break;
    default:
      break;
  }
  return ESP_OK;
}

void dns_server_task(void *pvParameters) {
  int socket_fd;
  struct sockaddr_in sa, ra;

  socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd < 0) {
    ESP_LOGE(TAG, "Failed to create socket");
    exit(0);
  }

  memset(&sa, 0, sizeof(struct sockaddr_in));

  tcpip_adapter_ip_info_t ip;
  tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip);
  ra.sin_family = AF_INET;
  ra.sin_addr.s_addr = ip.ip.addr;
  ra.sin_port = htons(53);
  if (bind(socket_fd, (struct sockaddr *)&ra, sizeof(struct sockaddr_in)) ==
      -1) {
    ESP_LOGE(TAG, "Failed to bind to 53/udp");
    close(socket_fd);
    exit(1);
  }

  struct sockaddr_in client;
  socklen_t client_len;
  client_len = sizeof(client);
  int length;
  char data[80];
  char response[100];
  char ipAddress[INET_ADDRSTRLEN];
  int idx;
  int err;

  ESP_LOGI(TAG, "DNS Server listening on 53/udp");
  while (1) {
    length = recvfrom(socket_fd, data, sizeof(data), 0,
                      (struct sockaddr *)&client, &client_len);
    if (length > 0) {
      data[length] = '\0';
      inet_ntop(AF_INET, &(client.sin_addr), ipAddress, INET_ADDRSTRLEN);
      ESP_LOGI(TAG, "Replying to DNS request (len=%d) from %s", length,
               ipAddress);

      // Prepare our response
      response[0] = data[0];
      response[1] = data[1];
      response[2] =
          0b10000100 |
          (0b00000001 & data[2]);  // response, authorative answer, not
                                   // truncated, copy the recursion bit
      response[3] = 0b00000000;    // no recursion available, no errors
      response[4] = data[4];
      response[5] = data[5];  // Question count
      response[6] = data[4];
      response[7] = data[5];  // answer count
      response[8] = 0x00;
      response[9] = 0x00;  // NS record count
      response[10] = 0x00;
      response[11] = 0x00;  // Resource record count

      memcpy(response + 12, data + 12,
             length - 12);  // Copy the rest of the query section
      idx = length;

      // Prune off the OPT
      // FIXME: We should parse the packet better than this!
      if ((response[idx - 11] == 0x00) && (response[idx - 10] == 0x00) &&
          (response[idx - 9] == 0x29))
        idx -= 11;

      // Set a pointer to the domain name in the question section
      response[idx] = 0xC0;
      response[idx + 1] = 0x0C;

      // Set the type to "Host Address"
      response[idx + 2] = 0x00;
      response[idx + 3] = 0x01;

      // Set the response class to IN
      response[idx + 4] = 0x00;
      response[idx + 5] = 0x01;

      // A 32 bit integer specifying TTL in seconds, 0 means no caching
      response[idx + 6] = 0x00;
      response[idx + 7] = 0x00;
      response[idx + 8] = 0x00;
      response[idx + 9] = 0x00;

      // RDATA length
      response[idx + 10] = 0x00;
      response[idx + 11] = 0x04;  // 4 byte IP address

      // The IP address
      response[idx + 12] = 192;
      response[idx + 13] = 168;
      response[idx + 14] = 4;
      response[idx + 15] = 1;

      err = sendto(socket_fd, response, idx + 16, 0, (struct sockaddr *)&client,
                   client_len);
      if (err < 0) {
        ESP_LOGE(TAG, "sendto failed: %s", strerror(errno));
      }
    } else {
      ESP_LOGI(TAG, "recv len = %d", length);
    }
  }
  close(socket_fd);
}
static void start_dns_server() {
  xTaskCreate(dns_server_task, "dns server", 3046, NULL, 5, &dns_server);
}
static void stop_dns_server() {
  vTaskDelete(dns_server);
  dns_server = NULL;
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
  esp_event_handler_instance_t instance_got_ip;

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
    if (esp_wifi_connect() != ESP_OK) {
      wifi_start_ap();
    }
  } else {
    wifi_start_ap();
  }
}
