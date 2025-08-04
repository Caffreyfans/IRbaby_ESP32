#include "conf.h"
#include "string.h"
#include <stdio.h>
#include "storage.h"

property_t ac_conf[] = {
    // {.key = "brand", .value = 4}, {.key = "protocol", .value = 2582}, {.key = "power", .value = 1}, {.key = "mode", .value = 1}, {.key = "temperature", .value = 26}, {.key = "fan", .value = 1}, {.key = "fan_speed", .value = 1}, {.key = "fan_direction", .value = 0}, {.key = "display", .value = 1}, {.key = "sleep", .value = 0}, {.key = "timer", .value = 0}};
    {.key = "brand", .value = 4}, {.key = "protocol", .value = 2582}, {.key = "power", .value = 1}, {.key = "mode", .value = 1}, {.key = "temperature", .value = 26}, {.key = "fan", .value = 1}, {.key = "fan_speed", .value = 1}, {.key = "fan_direction", .value = 0}};

property_t pin_conf[] = {
    {.key = "pin_ir_send", .value = 4},
    {.key = "pin_ir_recv", .value = 19},
    {.key = "pin_led", .value = 2},
    {.key = "pin_button", .value = 0},

};

void irbaby_set_conf(conf_type type, ac_ops key, int value)
{
  switch (type)
  {
  case CONF_AC:
    ac_conf[key].value = value;
    break;
  case CONF_PIN:
    pin_conf[key].value = value;
    break;
  default:
    break;
  }
  irbaby_store_conf();
}

property_t *irbaby_get_conf(conf_type type)
{
  property_t *property = NULL;
  switch (type)
  {
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

int irbaby_get_conf_num(conf_type type)
{
  int num = 0;
  switch (type)
  {
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

void irbaby_store_conf()
{
  irbaby_write("ac_config", (uint8_t *)&ac_conf, sizeof(ac_conf));
  irbaby_write("pin_config", (uint8_t *)&pin_conf, sizeof(pin_conf));
}

void irbaby_load_conf()
{
  uint8_t buffer[128];
  int len = 0;
  len = irbaby_read("ac_config", (uint8_t *)buffer, sizeof(buffer));
  if (len == sizeof(ac_conf))
  {
    memcpy(ac_conf, buffer, sizeof(ac_conf));
  }

  len = irbaby_read("pin_config", (uint8_t *)buffer, sizeof(buffer));
  if (len == sizeof(pin_conf))
  {
    memcpy(pin_conf, buffer, sizeof(pin_conf));
  }
}