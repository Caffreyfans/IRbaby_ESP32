#include "conf.h"
#include <stdio.h>
property_t ac_conf[] = {
    {.key = "brand", .value = 4},        {.key = "protocol", .value = 2582},
    {.key = "power", .value = 1},        {.key = "mode", .value = 1},
    {.key = "temperature", .value = 26}, {.key = "fan", .value = 1},
    {.key = "fan_speed", .value = 1},    {.key = "fan_direction", .value = 0}};

property_t pin_conf[] = {
    {.key = "pin_ir_send", .value = 14},
    {.key = "pin_ir_recv", .value = 12},
    {.key = "pin_led", .value = 2},
    {.key = "pin_button", .value = 0},

};

void irbaby_set_conf(conf_type type, ac_ops key, int value) {
  switch (type) {
    case CONF_AC:
      ac_conf[key].value = value;
      break;
    case CONF_PIN:
      pin_conf[key].value = value;
      break;
    default:
      break;
  }
}

property_t *irbaby_get_conf(conf_type type) {
  property_t *property = NULL;
  switch (type) {
    case CONF_AC:
      property = ac_conf;
      break;
    case CONF_PIN:
      property = pin_conf;
    default:
      break;
  }
  return property;
}

int irbaby_get_conf_num(conf_type type) {
  int num = 0;
  switch (type) {
    case CONF_AC:
      num = sizeof(ac_conf) / sizeof(ac_conf[0]);
      break;
    case CONF_PIN:
      num = sizeof(pin_conf) / sizeof(pin_conf[0]);
    default:
      break;
  }
  return num;
}