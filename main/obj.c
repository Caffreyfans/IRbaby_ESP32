#include "obj.h"
ac_property ac_obj[] = {
    {.key = "brand", .value = 1},        {.key = "protocol", .value = 0},
    {.key = "power", .value = 1},        {.key = "mode", .value = 1},
    {.key = "temperature", .value = 26}, {.key = "fan", .value = 1},
    {.key = "fan_speed", .value = 1},    {.key = "fan_direction", .value = 0}};

ac_property *get_ac_obj() { return &ac_obj; }

void set_ac_obj(ac_ops key, int value) { ac_obj[key].value = value; }
int get_ac_obj_property_len() { return sizeof(ac_obj) / sizeof(ac_obj[0]); }