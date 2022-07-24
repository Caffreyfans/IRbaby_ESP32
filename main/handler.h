/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 17:51:23
 * @LastEditTime: 2021-06-26 23:49:29
 * @Description: 
 */
#ifndef HANDLER_H_
#define HANDLER_H_
#include "esp_err.h"

char *connect_wifi_handle(const char *ssid, const char *pass);
char *web_get_index_handle();
char *scan_wifi_handle();
char *get_info_handle();
char *get_ir_handle();
char *get_gpio_handle();
char *get_more_handle();
#endif // HANDLER_H_