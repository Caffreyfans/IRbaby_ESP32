/*
 * @Author: Caffreyfans
 * @Date: 2021-06-20 10:33:01
 * @LastEditTime: 2021-06-26 21:58:17
 * @Description:
 */
#include "form_parser.h"

#include "esp_log.h"
#include "stdbool.h"
#include "stdio.h"
#include "string.h"
cJSON *form_parse(char *data) {
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
      has_item = true;
    }
  } while (index != NULL);
exit:
  if (has_item) return root;
  cJSON_Delete(root);
  return NULL;
}

// cJSON *get_parse(char *data) {
//   if (data == NULL) return NULL;
//   int data_len = strlen(data);
//   char *start = data;
//   char *end = NULL;
//   cJSON *root = cJSON_CreateObject();
//   if (root == NULL) return NULL;
//   char key[52];
//   char value[52];
//   while ((end = strchr(start, '=')) != NULL) {
//     int len = end - start;
//     memcpy(key, start, len);
//     char *next = strchr(index, '&');
//     if (next != NULL) {
//       end = next - 1;
//     } else {
//       end = data + data_len;
//     }
//     len = end - start;
//     memcpy(value, start, len);
//     cJSON_AddStringToObject(root, key, value);
//     start = end;
//   }
//   return root;
// }