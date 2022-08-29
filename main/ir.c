#include "ir.h"

#include "irbaby.h"
void ir_send() {
  property_t *ac_conf = irbaby_get_conf(CONF_AC);
  property_t *ac_pin = irbaby_get_conf(CONF_PIN);
  int protocol = ac_conf[CONF_AC_PROTOCOL].value;
  int pin = ac_pin[CONF_PIN_IR_SEND].value;
  t_remote_ac_status status = {
      .ac_display = ac_conf[CONF_AC_DISPLAY].value,
      .ac_mode = ac_conf[CONF_AC_MODE].value,
      .ac_power = ac_conf[CONF_AC_POWER].value,
      .ac_sleep = ac_conf[CONF_AC_SLEEP].value,
      .ac_temp = ac_conf[CONF_AC_TEMPERATURE].value,
      .ac_timer = ac_conf[CONF_AC_TIMER].value,
      .ac_wind_dir = ac_conf[CONF_AC_FAN_DIRECVTION].value,
      .ac_wind_speed = ac_conf[CONF_AC_FAN_SPEED].value};
  char protocol_name[32];
  snprintf(protocol_name, 32, "new_ac_%d", protocol);
  irbaby_send(protocol_name, &status);
}