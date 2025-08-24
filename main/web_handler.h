/*
 * @Author: Caffreyfans
 * @Date: 2021-06-19 17:51:23
 * @LastEditTime: 2021-06-26 23:49:29
 * @Description: this is web handler for response ajax
 */
#ifndef HANDLER_H_
#define HANDLER_H_
#include "conf.h"
#include "esp_err.h"

typedef enum
{
    SET_AC_BRAND,
    SET_AC_PROTOCOL,
    SET_AC_POWER,
    SET_AC_MODE,
    SET_AC_TEMPERATURE,
    SET_AC_FAN,
    SET_AC_FAN_SPEED,
    SET_AC_FAN_DIRECVTION,
    SET_AC_DISPLAY,
    SET_AC_SLEEP,
    SET_AC_TIMER,
    SET_IR_RECEIVE_ENABLE,
    SEND_IR_INCOMING
} ir_ops;

char *connect_wifi_handle(const char *ssid, const char *pass);
char *get_index_handle();
char *scan_wifi_handle();

char *set_ir_handle(ir_ops ops, int value);
char *get_ir_handle();
char *set_gpio_handle(pin_ops ops, int value);
char *get_gpio_handle();
char *get_info_handle();
char *get_more_handle();
#endif // HANDLER_H_