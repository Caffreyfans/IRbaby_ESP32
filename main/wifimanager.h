/*
 * @Author: Caffreyfans
 * @Date: 2021-06-20 14:16:09
 * @LastEditTime: 2021-06-26 23:16:06
 * @Description:
 */
#ifndef WIFI_MANAGER_H_
#define WIFI_MANAGER_H_
#include <stdbool.h>
#include <stdint.h>

#include "esp_wifi.h"

void wifi_init();
bool wifi_start_ap();
bool wifi_start_station(const char *ssid, const char *pass);
uint8_t wifi_scan_ap(wifi_ap_record_t *ap_infos, const uint8_t size);

extern esp_netif_t *g_station_netif;
#endif  // WIFI_MANAGER_H_
