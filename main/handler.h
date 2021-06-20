/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 17:51:23
 * @LastEditTime: 2021-06-20 14:41:28
 * @Description: 
 */
#ifndef HANDLER_H_
#define HANDLER_H_
#include "cJSON.h"
#include "esp_err.h"

char *connect_wifi_handle(const char *ssid, const char *pass);
char *scan_wifi_handle();
#endif // HANDLER_H_