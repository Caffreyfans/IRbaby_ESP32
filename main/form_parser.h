/*
 * @Author: Caffreyfans
 * @Date: 2021-06-20 10:32:53
 * @LastEditTime: 2021-06-23 22:38:43
 * @Description:
 */
#ifndef FORM_PARSER_H_
#define FORM_PARSER_H_
#include "cJSON.h"
cJSON* form_parse(char* data);
cJSON* get_parse(char* data);
#endif
