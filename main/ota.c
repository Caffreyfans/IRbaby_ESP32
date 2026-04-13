/*
 * OTA update handler
 */
#include "esp_http_client.h"
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ota";

static void ota_reboot_task(void *arg)
{
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();
}

char *ota_update_handle(const char *url)
{
  char *response = NULL;
  cJSON *root = cJSON_CreateObject();
  if (root == NULL) return NULL;

  esp_http_client_config_t config = {
      .url = url,
      .method = HTTP_METHOD_GET,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL) {
    cJSON_AddStringToObject(root, "status", "error");
    cJSON_AddStringToObject(root, "message", "Failed to init HTTP client");
    response = cJSON_Print(root);
    cJSON_Delete(root);
    return response;
  }

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    cJSON_AddStringToObject(root, "status", "error");
    char msg[64];
    snprintf(msg, 64, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    cJSON_AddStringToObject(root, "message", msg);
    response = cJSON_Print(root);
    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    return response;
  }

  esp_ota_handle_t update_handle = 0;
  const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);
  if (update_partition == NULL) {
    cJSON_AddStringToObject(root, "status", "error");
    cJSON_AddStringToObject(root, "message", "No OTA partition available");
    response = cJSON_Print(root);
    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    return response;
  }

  err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
  if (err != ESP_OK) {
    cJSON_AddStringToObject(root, "status", "error");
    char msg[64];
    snprintf(msg, 64, "Failed to begin OTA: %s", esp_err_to_name(err));
    cJSON_AddStringToObject(root, "message", msg);
    response = cJSON_Print(root);
    cJSON_Delete(root);
    esp_http_client_cleanup(client);
    return response;
  }

  int total_read_len = 0;
  char buffer[1024];
  while (1) {
    int read_len = esp_http_client_read(client, buffer, sizeof(buffer));
    if (read_len < 0) {
      ESP_LOGE(TAG, "Error read data");
      err = ESP_FAIL;
      break;
    } else if (read_len == 0) {
      break;
    }
    err = esp_ota_write(update_handle, buffer, read_len);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "Error write OTA data");
      break;
    }
    total_read_len += read_len;
    ESP_LOGI(TAG, "Written %d bytes", total_read_len);
  }

  esp_http_client_close(client);
  esp_http_client_cleanup(client);

  if (err == ESP_OK) {
    err = esp_ota_end(update_handle);
    if (err == ESP_OK) {
      err = esp_ota_set_boot_partition(update_partition);
      if (err == ESP_OK) {
        cJSON_AddStringToObject(root, "status", "success");
        if (xTaskCreate(ota_reboot_task, "ota_reboot_task", 2048, NULL, 5, NULL) == pdPASS) {
          cJSON_AddStringToObject(root, "message", "OTA update successful, rebooting...");
        } else {
          cJSON_AddStringToObject(root, "message", "OTA update successful, reboot scheduling failed");
        }
      } else {
        cJSON_AddStringToObject(root, "status", "error");
        cJSON_AddStringToObject(root, "message", "Failed to set boot partition");
      }
    } else {
      cJSON_AddStringToObject(root, "status", "error");
      cJSON_AddStringToObject(root, "message", "Failed to end OTA");
    }
  } else {
    esp_ota_abort(update_handle);
    cJSON_AddStringToObject(root, "status", "error");
    cJSON_AddStringToObject(root, "message", "OTA update failed");
  }

  response = cJSON_Print(root);
  cJSON_Delete(root);
  return response;
}
