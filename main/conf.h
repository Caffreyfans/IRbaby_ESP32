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

typedef struct
{
  uint16_t key;
  uint16_t value;
  char *label;
} property_t;

typedef struct
{
  uint16_t key;
  char *value;
  char *label;
} string_property_t;

typedef enum
{
  CONF_AC,
  CONF_PIN,
  CONF_MQTT,
  CONF_HOMEKIT,
  CONFIG_INFO,
  CONFIG_MORE
} conf_type;

typedef enum
{
  CONF_AC_BRAND,
  CONF_AC_PROTOCOL,
  CONF_AC_POWER,
  CONF_AC_MODE,
  CONF_AC_TEMPERATURE,
  CONF_AC_FAN,
  CONF_AC_FAN_SPEED,
  CONF_AC_FAN_DIRECVTION,
  CONF_AC_DISPLAY,
  CONF_AC_SLEEP,
  CONF_AC_TIMER,
  CONF_AC_MAX,
} ac_ops;

typedef enum
{
  CONF_PIN_IR_SEND,
  CONF_PIN_IR_RECV,
  CONF_PIN_LED,
  CONF_PIN_BUTTON,
  CONF_PIN_MAX,
} pin_ops;

typedef enum
{
  CONF_MQTT_ENABLE,
  CONF_MQTT_BROKER_URL,
  CONF_MQTT_BROKER_PORT,
  CONF_MQTT_USERNAME,
  CONF_MQTT_PASSWORD,
  CONF_MQTT_CLIENT_ID,
  CONF_MQTT_TOPIC_PREFIX,
  CONF_MQTT_MAX,
} mqtt_ops;

typedef enum
{
  CONF_HOMEKIT_ENABLE,
  CONF_HOMEKIT_SETUP_CODE,
  CONF_HOMEKIT_MAX,
} homekit_ops;

void irbaby_set_conf(conf_type type, int key, int value);
void irbaby_set_string_conf(conf_type type, int key, const char *value);
property_t *irbaby_get_conf(conf_type type);
string_property_t *irbaby_get_string_conf(conf_type type);
int irbaby_get_conf_num(conf_type type);
void irbaby_store_conf();
void irbaby_load_conf();
char* irbaby_get_conf_label(conf_type type, int key);
#endif // CONF_H_