/**
 * @file obj.h
 * @author Caffreyfans
 * @brief
 * @version 0.1
 * @date 2022-07-24
 *
 * @copyright Copyright (c) 2022
 *
 */
#ifndef CONF_H_
#define CONF_H_
#include <stdint.h>

typedef struct {
  char *key;
  uint16_t value;
} property_t;

typedef enum { CONF_AC, CONF_PIN, CONFIG_INFO, CONFIG_MORE } conf_type;

typedef enum {
  CONF_AC_BRAND,
  CONF_AC_PROTOCOL,
  CONF_AC_POWER,
  CONF_AC_MODE,
  CONF_AC_TEMPERATURE,
  CONF_AC_FAN,
  CONF_AC_FAN_SPEED,
  CONF_AC_FAN_DIRECVTION,
} ac_ops;

typedef enum {
  CONF_PIN_IR_SEND,
  CONF_PIN_IR_RECV,
  CONF_PIN_LED,
  CONF_PIN_BUTTON
} pin_ops;

void irbaby_set_conf(conf_type type, ac_ops key, int value);
property_t *irbaby_get_conf(conf_type type);
int irbaby_get_conf_num(conf_type type);
#endif  // CONF_H_