#include "conf.h"
#include "string.h"
#include <stdio.h>
#include "storage.h"
#include "esp_log.h"
#include "cJSON.h"

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

char *mqtt_conf_label[CONF_MQTT_MAX] = {
    "mqtt_enable",
    "mqtt_broker_url",
    "mqtt_broker_port",
    "mqtt_username",
    "mqtt_password",
    "mqtt_client_id",
    "mqtt_topic_prefix",
};

char *homekit_conf_label[CONF_HOMEKIT_MAX] = {
    "homekit_enable",
    "homekit_setup_code",
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

string_property_t mqtt_conf[CONF_MQTT_MAX] = {
    {.key = CONF_MQTT_ENABLE, .value = "0"},
    {.key = CONF_MQTT_BROKER_URL, .value = ""},
    {.key = CONF_MQTT_BROKER_PORT, .value = "1883"},
    {.key = CONF_MQTT_USERNAME, .value = ""},
    {.key = CONF_MQTT_PASSWORD, .value = ""},
    {.key = CONF_MQTT_CLIENT_ID, .value = "IRbaby"},
    {.key = CONF_MQTT_TOPIC_PREFIX, .value = "irbaby"},
};

string_property_t homekit_conf[CONF_HOMEKIT_MAX] = {
    {.key = CONF_HOMEKIT_ENABLE, .value = "0"},
    {.key = CONF_HOMEKIT_SETUP_CODE, .value = "12345678"},
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
  else if (type == CONF_MQTT)
  {
    return mqtt_conf_label[key];
  }
  else if (type == CONF_HOMEKIT)
  {
    return homekit_conf_label[key];
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

void irbaby_set_string_conf(conf_type type, int key, const char *value)
{
  switch (type)
  {
  case CONF_MQTT:
    if (mqtt_conf[key].value) {
      free(mqtt_conf[key].value);
    }
    mqtt_conf[key].value = strdup(value);
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
    break;
  default:
    break;
  }
  return property;
}

string_property_t *irbaby_get_string_conf(conf_type type)
{
  string_property_t *property = NULL;
  switch (type)
  {
  case CONF_MQTT:
    property = mqtt_conf;
    break;
  case CONF_HOMEKIT:
    property = homekit_conf;
    break;
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
    break;
  case CONF_MQTT:
    num = sizeof(mqtt_conf) / sizeof(mqtt_conf[0]);
    break;
  case CONF_HOMEKIT:
    num = sizeof(homekit_conf) / sizeof(homekit_conf[0]);
    break;
  default:
    break;
  }
  return num;
}

void irbaby_store_conf()
{
  irbaby_write("ac_config", (uint8_t *)&ac_conf, sizeof(ac_conf));
  irbaby_write("pin_config", (uint8_t *)&pin_conf, sizeof(pin_conf));
  // Store MQTT config as JSON
  cJSON *mqtt_root = cJSON_CreateObject();
  for (int i = 0; i < CONF_MQTT_MAX; i++) {
    cJSON_AddStringToObject(mqtt_root, mqtt_conf_label[i], mqtt_conf[i].value ? mqtt_conf[i].value : "");
  }
  char *mqtt_json = cJSON_PrintUnformatted(mqtt_root);
  irbaby_write("mqtt_config", (uint8_t *)mqtt_json, strlen(mqtt_json) + 1);
  free(mqtt_json);
  cJSON_Delete(mqtt_root);
  
  // Store HomeKit config as JSON
  cJSON *homekit_root = cJSON_CreateObject();
  for (int i = 0; i < CONF_HOMEKIT_MAX; i++) {
    cJSON_AddStringToObject(homekit_root, homekit_conf_label[i], homekit_conf[i].value ? homekit_conf[i].value : "");
  }
  char *homekit_json = cJSON_PrintUnformatted(homekit_root);
  irbaby_write("homekit_config", (uint8_t *)homekit_json, strlen(homekit_json) + 1);
  free(homekit_json);
  cJSON_Delete(homekit_root);
}

void irbaby_load_conf()
{
  uint8_t buffer[1024];
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
  for (int i = 0; i < CONF_MQTT_MAX; i++)
  {
    mqtt_conf[i].label = mqtt_conf_label[i];
    ESP_LOGI(TAG, "%d, %s", i, mqtt_conf[i].label);
  }
  for (int i = 0; i < CONF_HOMEKIT_MAX; i++)
  {
    homekit_conf[i].label = homekit_conf_label[i];
    ESP_LOGI(TAG, "%d, %s", i, homekit_conf[i].label);
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

  len = irbaby_read("mqtt_config", (uint8_t *)buffer, sizeof(buffer));
  if (len > 0) {
    cJSON *mqtt_root = cJSON_Parse((char *)buffer);
    if (mqtt_root) {
      for (int i = 0; i < CONF_MQTT_MAX; i++) {
        cJSON *item = cJSON_GetObjectItem(mqtt_root, mqtt_conf_label[i]);
        if (item && cJSON_IsString(item)) {
          if (mqtt_conf[i].value) {
            free(mqtt_conf[i].value);
          }
          mqtt_conf[i].value = strdup(item->valuestring);
        }
      }
      cJSON_Delete(mqtt_root);
    }
  }

  len = irbaby_read("homekit_config", (uint8_t *)buffer, sizeof(buffer));
  if (len > 0) {
    cJSON *homekit_root = cJSON_Parse((char *)buffer);
    if (homekit_root) {
      for (int i = 0; i < CONF_HOMEKIT_MAX; i++) {
        cJSON *item = cJSON_GetObjectItem(homekit_root, homekit_conf_label[i]);
        if (item && cJSON_IsString(item)) {
          if (homekit_conf[i].value) {
            free(homekit_conf[i].value);
          }
          homekit_conf[i].value = strdup(item->valuestring);
        }
      }
      cJSON_Delete(homekit_root);
    }
  }
}