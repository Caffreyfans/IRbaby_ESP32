#include "ir.h"

#include "irbaby.h"
#include "esp_log.h"
#include "peripherals.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
static const char *TAG = "IR";
TaskHandle_t ir_receive_task_handle = NULL;

int ir_transmission(ir_send_type send_type)
{
  if (send_type == IR_SEND_DECODE)
  {
    property_t *ac_conf = irbaby_get_conf(CONF_AC);
    property_t *ac_pin = irbaby_get_conf(CONF_PIN);
    int protocol = ac_conf[CONF_AC_PROTOCOL].value;
    int pin = ac_pin[CONF_PIN_IR_SEND].value;
    t_remote_ac_status status = {
        .ac_display = ac_conf[CONF_AC_DISPLAY].value,
        .ac_mode = ac_conf[CONF_AC_MODE].value,
        .ac_power = !ac_conf[CONF_AC_POWER].value,
        .ac_sleep = ac_conf[CONF_AC_SLEEP].value,
        .ac_temp = ac_conf[CONF_AC_TEMPERATURE].value,
        .ac_timer = ac_conf[CONF_AC_TIMER].value,
        .ac_wind_dir = ac_conf[CONF_AC_FAN_DIRECVTION].value,
        .ac_wind_speed = ac_conf[CONF_AC_FAN_SPEED].value};
    char protocol_name[32];
    snprintf(protocol_name, 32, "new_ac_%d", protocol);
    irbaby_send(protocol_name, &status, pin, ac_conf[CONF_AC_FAN].value);
  }
  else if (send_type == IR_SEND_INCOMING)
  {
    ir_send_incoming();
  }
  return 0;
}

void ir_receive_task(void *pvParamTer)
{
  ir_receive();
}

void ir_receive_start()
{
  if (ir_receive_task_handle == NULL)
  {
    xTaskCreate(&ir_receive_task, "ir_receive", 4096, NULL, 5, &ir_receive_task_handle);
  }
  else
  {
    vTaskResume(ir_receive_task_handle);
  }
}
void ir_receive_stop()
{
  if (ir_receive_task_handle != NULL)
  {
    vTaskSuspend(ir_receive_task_handle);
  }
}
