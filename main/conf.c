#include "conf.h"
#include "string.h"
#include <stdio.h>
#include "storage.h"
#include "esp_log.h"

#define TAG "conf"

char *ac_conf_label[CONF_AC_MAX] = {
    "brand",
    "protocol",
    "power",
    "mode",
    "temperature",
    "fan",
    "fan_speed",
    "fan_direction",
    "display",
    "sleep",
    "timer",
};

char *pin_conf_label[CONF_PIN_MAX] = {
    "pin_ir_send",
    "pin_ir_recv",
    "pin_led",
    "pin_button",
};

property_t ac_conf[CONF_AC_MAX] = {
    {.key = CONF_AC_BRAND, .value = 4},
    {.key = CONF_AC_PROTOCOL, .value = 2582},
    {.key = CONF_AC_POWER, .value = 0},
    {.key = CONF_AC_MODE, .value = 1},
    {.key = CONF_AC_TEMPERATURE, .value = 0},
    {.key = CONF_AC_FAN, .value = 1},
    {.key = CONF_AC_FAN_SPEED, .value = 1},
    {.key = CONF_AC_FAN_DIRECVTION, .value = 1},
    {.key = CONF_AC_DISPLAY, .value = 0},
    {.key = CONF_AC_SLEEP, .value = 0},
    {.key = CONF_AC_TIMER, .value = 0}};

property_t pin_conf[CONF_PIN_MAX] = {
    {.key = CONF_PIN_IR_SEND, .value = 4},
    {.key = CONF_PIN_IR_RECV, .value = 19},
    {.key = CONF_PIN_LED, .value = 2},
    {.key = CONF_PIN_BUTTON, .value = 0},
};

char *irbaby_get_conf_label(conf_type type, int key)
{
  if (type == CONF_AC)
  {
    return ac_conf_label[key];
  }
  else if (type == CONF_PIN)
  {
    return pin_conf_label[key];
  }
  return NULL;
}

void irbaby_set_conf(conf_type type, int key, int value)
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
  uint8_t buffer[256];
  int len = 0;
  for (int i = 0; i < CONF_AC_MAX; i++)
  {
    ac_conf[i].label = ac_conf_label[i];
    ESP_LOGI(TAG, "%d, %s", i, ac_conf[i].label);
  }
  for (int i = 0; i < CONF_PIN_MAX; i++)
  {
    pin_conf[i].label = pin_conf_label[i];
    ESP_LOGI(TAG, "%d, %s", i, pin_conf[i].label);
  }

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