/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 17:51:23
 * @LastEditTime: 2021-06-26 23:49:29
 * @Description:
 */
#ifndef HANDLER_H_
#define HANDLER_H_
#include "conf.h"
#include "esp_err.h"

char *get_protocol_handle(int brand_id);
char *connect_wifi_handle(const char *ssid, const char *pass);
char *get_index_handle();
char *scan_wifi_handle();

char *set_ir_handle(ac_ops ops, int value);
char *get_ir_handle();
char *set_gpio_handle(pin_ops ops, int value);
char *get_gpio_handle();
char *get_info_handle();
char *get_more_handle();
#endif  // HANDLER_H_