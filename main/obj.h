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
#ifndef OBJ_H_
#define OBJ_H_
#include <stdint.h>
typedef struct {
  char *key;
  uint16_t value;
} ac_property;

typedef enum {
  AC_BRAND,
  AC_PROTOCOL,
  AC_POWER,
  AC_MODE,
  AC_TEMPERATURE,
  AC_FAN,
  AC_FAN_SPEED,
  AC_FAN_DIRECVTION,
} ac_ops;

void set_ac_obj(ac_ops key, int value);
ac_property *get_ac_obj();
int get_ac_obj_property_len();
#endif  // OBJ_H_