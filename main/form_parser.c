/*
 * @Author: Caffreyfans
 * @Date: 2021-06-20 10:33:01
 * @LastEditTime: 2021-06-20 12:21:34
 * @Description:
 */
#include "form_parser.h"

#include "esp_log.h"
#include "stdbool.h"
#include "string.h"
#include "stdio.h"
cJSON *form_parse(const char *data) {
  char *key = "name=\"";
  int key_len = strlen(key);
  char *index = NULL;
  char name[128] = {0};
  char value[128] = {0};
  cJSON *root = cJSON_CreateObject();
  bool has_item = false;
  if (root == NULL) goto exit;
  do {
    index = strstr(data, key);
    if (index == NULL) goto exit;
    /* get name */
    char *start = index + key_len;
    char *end = strchr(start, '"');
    int len = end - start;
    memcpy(name, start, len);
    name[len] = '\0';
    data = end;
    /* get value */
    start = data + 5;
    end = strstr(start, "\r\n");
    len = end - start;
    memcpy(value, start, len);
    value[len] = '\0';
    data = end;
    if (index != NULL) {
      cJSON_AddStringToObject(root, name, value);
      printf("get name = %s\n", name);
      printf("get value = %s\n", value);
      has_item = true;
    }
  } while (index != NULL);
exit:
  if (has_item) return root;
  cJSON_Delete(root);
  return NULL;
}
