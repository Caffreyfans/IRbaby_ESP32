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
char *get_mqtt_handle();
char *set_mqtt_handle(mqtt_ops ops, const char *value);
char *get_info_handle();
char *get_more_handle();
char *ota_update_handle(const char *url);
char *reboot_handle();
char *get_file_list_handle();
char *delete_file_handle(const char *filename);
char *get_homekit_handle();
char *set_homekit_handle(int ops, const char *value);
#endif // HANDLER_H_