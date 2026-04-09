/*
 * MQTT client for IRbaby
 */
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include "ir.h"
#include "conf.h"
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

// Forward declarations
static void mqtt_build_topics(const char *prefix);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
static void mqtt_publish_ha_discovery(void);
void mqtt_publish_status();

static const char *TAG = "mqtt";

esp_mqtt_client_handle_t mqtt_client = NULL;
// Mutex to protect global MQTT topic strings
static SemaphoreHandle_t mqtt_mutex = NULL;
static char *mqtt_topic_prefix = NULL;
static char *mqtt_command_topic = NULL;
static char *mqtt_state_topic = NULL;
static char *mqtt_response_topic = NULL;
static char *mqtt_ha_discovery_topic = NULL;

// Initialize mutex for thread safety
static esp_err_t mqtt_init_mutex(void)
{
    if (mqtt_mutex == NULL) {
        mqtt_mutex = xSemaphoreCreateMutex();
        if (mqtt_mutex == NULL) {
            ESP_LOGE(TAG, "Failed to create MQTT mutex");
            return ESP_ERR_NO_MEM;
        }
    }
    return ESP_OK;
}

// Lock with error handling
static void mqtt_lock(void)
{
    if (mqtt_mutex) {
        xSemaphoreTake(mqtt_mutex, portMAX_DELAY);
    }
}

// Unlock 
static void mqtt_unlock(void)
{
    if (mqtt_mutex) {
        xSemaphoreGive(mqtt_mutex);
    }
}

static void mqtt_build_topics(const char *prefix)
{
    mqtt_lock();
    
    const char *base = (prefix && prefix[0]) ? prefix : "irbaby";
    free(mqtt_topic_prefix);
    free(mqtt_command_topic);
    free(mqtt_state_topic);
    free(mqtt_response_topic);
    free(mqtt_ha_discovery_topic);

    mqtt_topic_prefix = strdup(base);
    if (!mqtt_topic_prefix) {
        mqtt_topic_prefix = strdup("irbaby");
        if (!mqtt_topic_prefix) {
            ESP_LOGE(TAG, "Failed to allocate mqtt_topic_prefix");
            mqtt_unlock();
            return;
        }
    }

    int len = snprintf(NULL, 0, "%s/command", mqtt_topic_prefix) + 1;
    mqtt_command_topic = malloc(len);
    if (!mqtt_command_topic) {
        ESP_LOGE(TAG, "Failed to allocate mqtt_command_topic");
        mqtt_unlock();
        return;
    }
    snprintf(mqtt_command_topic, len, "%s/command", mqtt_topic_prefix);

    len = snprintf(NULL, 0, "%s/state", mqtt_topic_prefix) + 1;
    mqtt_state_topic = malloc(len);
    if (!mqtt_state_topic) {
        ESP_LOGE(TAG, "Failed to allocate mqtt_state_topic");
        mqtt_unlock();
        return;
    }
    snprintf(mqtt_state_topic, len, "%s/state", mqtt_topic_prefix);

    len = snprintf(NULL, 0, "%s/response", mqtt_topic_prefix) + 1;
    mqtt_response_topic = malloc(len);
    if (!mqtt_response_topic) {
        ESP_LOGE(TAG, "Failed to allocate mqtt_response_topic");
        mqtt_unlock();
        return;
    }
    snprintf(mqtt_response_topic, len, "%s/response", mqtt_topic_prefix);

    // Home Assistant discovery topic
    len = snprintf(NULL, 0, "homeassistant/climate/%s/config", mqtt_topic_prefix) + 1;
    mqtt_ha_discovery_topic = malloc(len);
    if (!mqtt_ha_discovery_topic) {
        ESP_LOGE(TAG, "Failed to allocate mqtt_ha_discovery_topic");
        mqtt_unlock();
        return;
    }
    snprintf(mqtt_ha_discovery_topic, len, "homeassistant/climate/%s/config", mqtt_topic_prefix);
    
    mqtt_unlock();
}

static void mqtt_publish_ha_discovery(void)
{
    if (!mqtt_client || !mqtt_ha_discovery_topic) {
        return;
    }

    cJSON *discovery = cJSON_CreateObject();
    if (!discovery) {
        ESP_LOGE(TAG, "Failed to create HA discovery JSON");
        return;
    }

    // Basic device info
    cJSON_AddStringToObject(discovery, "name", "IRbaby AC");
    cJSON_AddStringToObject(discovery, "unique_id", "irbaby_ac");
    cJSON_AddStringToObject(discovery, "device_class", "climate");
    
    // State and command topics
    cJSON_AddStringToObject(discovery, "state_topic", mqtt_state_topic);
    cJSON_AddStringToObject(discovery, "command_topic", mqtt_command_topic);
    
    // Temperature settings
    cJSON_AddNumberToObject(discovery, "min_temp", 16);
    cJSON_AddNumberToObject(discovery, "max_temp", 30);
    cJSON_AddNumberToObject(discovery, "temp_step", 1);
    cJSON_AddStringToObject(discovery, "temperature_unit", "C");
    cJSON_AddStringToObject(discovery, "temperature_state_topic", mqtt_state_topic);
    cJSON_AddStringToObject(discovery, "temperature_command_topic", mqtt_command_topic);
    
    // Modes
    cJSON *modes = cJSON_CreateArray();
    cJSON_AddItemToArray(modes, cJSON_CreateString("off"));
    cJSON_AddItemToArray(modes, cJSON_CreateString("auto"));
    cJSON_AddItemToArray(modes, cJSON_CreateString("cool"));
    cJSON_AddItemToArray(modes, cJSON_CreateString("heat"));
    cJSON_AddItemToArray(modes, cJSON_CreateString("dry"));
    cJSON_AddItemToArray(modes, cJSON_CreateString("fan_only"));
    cJSON_AddItemToObject(discovery, "modes", modes);
    
    // Fan modes
    cJSON *fan_modes = cJSON_CreateArray();
    cJSON_AddItemToArray(fan_modes, cJSON_CreateString("auto"));
    cJSON_AddItemToArray(fan_modes, cJSON_CreateString("low"));
    cJSON_AddItemToArray(fan_modes, cJSON_CreateString("medium"));
    cJSON_AddItemToArray(fan_modes, cJSON_CreateString("high"));
    cJSON_AddItemToObject(discovery, "fan_modes", fan_modes);
    
    // Payloads
    cJSON_AddStringToObject(discovery, "payload_on", "on");
    cJSON_AddStringToObject(discovery, "payload_off", "off");
    
    // Device info
    cJSON *device = cJSON_CreateObject();
    cJSON_AddStringToObject(device, "name", "IRbaby ESP32");
    cJSON_AddStringToObject(device, "model", "IRbaby AC Controller");
    cJSON_AddStringToObject(device, "manufacturer", "IRbaby");
    cJSON *identifiers = cJSON_CreateArray();
    cJSON_AddItemToArray(identifiers, cJSON_CreateString("irbaby_ac"));
    cJSON_AddItemToObject(device, "identifiers", identifiers);
    cJSON_AddItemToObject(discovery, "device", device);

    char *payload = cJSON_PrintUnformatted(discovery);
    if (payload) {
        esp_mqtt_client_publish(mqtt_client, mqtt_ha_discovery_topic, payload, 0, 1, 0);
        free(payload);
        ESP_LOGI(TAG, "Published HA discovery message to %s", mqtt_ha_discovery_topic);
    } else {
        ESP_LOGE(TAG, "Failed to serialize HA discovery JSON");
    }

    cJSON_Delete(discovery);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // Make local copies under lock to avoid using-after-free
        mqtt_lock();
        char *state_topic_copy = mqtt_state_topic ? strdup(mqtt_state_topic) : NULL;
        char *command_topic_copy = mqtt_command_topic ? strdup(mqtt_command_topic) : NULL;
        mqtt_unlock();
        
        if (state_topic_copy) {
            msg_id = esp_mqtt_client_publish(client, state_topic_copy, "online", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            free(state_topic_copy);
        }

        if (command_topic_copy) {
            msg_id = esp_mqtt_client_subscribe(client, command_topic_copy, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
            free(command_topic_copy);
        }

        // Publish Home Assistant discovery message
        mqtt_publish_ha_discovery();
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);

        // Parse command
        cJSON *root = cJSON_ParseWithLength(event->data, event->data_len);
        if (root != NULL) {
            cJSON *cmd = cJSON_GetObjectItem(root, "command");
            if (cJSON_IsString(cmd)) {
                if (strcmp(cmd->valuestring, "send_ir") == 0) {
                    // Send IR
                    ir_transmission(IR_SEND_DECODE);
                    esp_mqtt_client_publish(client, "irbaby/response", "{\"status\":\"ok\"}", 0, 0, 0);
                } else if (strcmp(cmd->valuestring, "send_incoming") == 0) {
                    ir_transmission(IR_SEND_INCOMING);
                    esp_mqtt_client_publish(client, "irbaby/response", "{\"status\":\"ok\"}", 0, 0, 0);
                }
            } else {
                // Handle Home Assistant climate commands
                property_t *ac_conf = irbaby_get_conf(CONF_AC);
                if (ac_conf) {
                    bool updated = false;
                    
                    // Handle mode
                    cJSON *mode = cJSON_GetObjectItem(root, "mode");
                    if (cJSON_IsString(mode)) {
                        if (strcmp(mode->valuestring, "off") == 0) {
                            ac_conf[CONF_AC_POWER].value = 0;
                            updated = true;
                        } else if (strcmp(mode->valuestring, "cool") == 0) {
                            ac_conf[CONF_AC_POWER].value = 1;
                            ac_conf[CONF_AC_MODE].value = 1; // cool mode
                            updated = true;
                        } else if (strcmp(mode->valuestring, "heat") == 0) {
                            ac_conf[CONF_AC_POWER].value = 1;
                            ac_conf[CONF_AC_MODE].value = 2; // heat mode
                            updated = true;
                        } else if (strcmp(mode->valuestring, "auto") == 0) {
                            ac_conf[CONF_AC_POWER].value = 1;
                            ac_conf[CONF_AC_MODE].value = 0; // auto mode
                            updated = true;
                        } else if (strcmp(mode->valuestring, "dry") == 0) {
                            ac_conf[CONF_AC_POWER].value = 1;
                            ac_conf[CONF_AC_MODE].value = 3; // dry mode
                            updated = true;
                        } else if (strcmp(mode->valuestring, "fan_only") == 0) {
                            ac_conf[CONF_AC_POWER].value = 1;
                            ac_conf[CONF_AC_MODE].value = 4; // fan only mode
                            updated = true;
                        }
                    }
                    
                    // Handle temperature
                    cJSON *temp = cJSON_GetObjectItem(root, "temperature");
                    if (cJSON_IsNumber(temp)) {
                        ac_conf[CONF_AC_TEMPERATURE].value = (int)temp->valuedouble;
                        updated = true;
                    }
                    
                    // Handle fan mode
                    cJSON *fan_mode = cJSON_GetObjectItem(root, "fan_mode");
                    if (cJSON_IsString(fan_mode)) {
                        if (strcmp(fan_mode->valuestring, "auto") == 0) {
                            ac_conf[CONF_AC_FAN_SPEED].value = 0;
                            updated = true;
                        } else if (strcmp(fan_mode->valuestring, "low") == 0) {
                            ac_conf[CONF_AC_FAN_SPEED].value = 1;
                            updated = true;
                        } else if (strcmp(fan_mode->valuestring, "medium") == 0) {
                            ac_conf[CONF_AC_FAN_SPEED].value = 2;
                            updated = true;
                        } else if (strcmp(fan_mode->valuestring, "high") == 0) {
                            ac_conf[CONF_AC_FAN_SPEED].value = 3;
                            updated = true;
                        }
                    }
                    
                    if (updated) {
                        // Save individual AC properties
                        irbaby_set_conf(CONF_AC, CONF_AC_POWER, ac_conf[CONF_AC_POWER].value);
                        irbaby_set_conf(CONF_AC, CONF_AC_MODE, ac_conf[CONF_AC_MODE].value);
                        irbaby_set_conf(CONF_AC, CONF_AC_TEMPERATURE, ac_conf[CONF_AC_TEMPERATURE].value);
                        irbaby_set_conf(CONF_AC, CONF_AC_FAN_SPEED, ac_conf[CONF_AC_FAN_SPEED].value);
                        ir_transmission(IR_SEND_DECODE);
                        mqtt_publish_status();
                        esp_mqtt_client_publish(client, "irbaby/response", "{\"status\":\"ok\"}", 0, 0, 0);
                    }
                }
            }
            cJSON_Delete(root);
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

esp_err_t mqtt_init(const char *broker_url, int port, const char *username, const char *password, const char *client_id, const char *topic_prefix)
{
    // Initialize mutex early
    if (mqtt_init_mutex() != ESP_OK) {
        return ESP_ERR_NO_MEM;
    }
    
    if (mqtt_client != NULL) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
    }

    if (!broker_url || strlen(broker_url) == 0) {
        ESP_LOGE(TAG, "MQTT broker URL is empty");
        return ESP_ERR_INVALID_ARG;
    }

    char *broker_uri = NULL;
    const char *host_start = strstr(broker_url, "://");
    if (host_start) {
        host_start += 3;
    } else {
        host_start = broker_url;
    }

    bool has_port = false;
    const char *p = host_start;
    while (*p && *p != '/' && *p != ':') {
        p++;
    }
    if (*p == ':') {
        has_port = true;
    }

    if (port > 0 && !has_port) {
        const char *path = strchr(host_start, '/');
        size_t prefix_len = path ? (path - broker_url) : strlen(broker_url);
        size_t extra = 1 + 6 + (path ? strlen(path) : 0);
        broker_uri = malloc(prefix_len + extra + 1);
        if (broker_uri) {
            memcpy(broker_uri, broker_url, prefix_len);
            int offset = prefix_len;
            offset += snprintf(broker_uri + offset, extra + 1, ":%d", port);
            if (path) {
                strcpy(broker_uri + offset, path);
            } else {
                broker_uri[offset] = '\0';
            }
        }
    } else {
        broker_uri = strdup(broker_url);
    }

    if (!broker_uri) {
        ESP_LOGE(TAG, "Failed to allocate MQTT broker URI");
        return ESP_ERR_NO_MEM;
    }

    mqtt_build_topics(topic_prefix);

    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = broker_uri;
    mqtt_cfg.credentials.username = username;
    mqtt_cfg.credentials.authentication.password = password;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    free(broker_uri);

    if (mqtt_client) {
        esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
        esp_mqtt_client_start(mqtt_client);
        return ESP_OK;
    } else {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return ESP_FAIL;
    }
}

void mqtt_publish_status()
{
    if (mqtt_client != NULL) {
        property_t *ac_conf = irbaby_get_conf(CONF_AC);
        if (!ac_conf) {
            return;
        }
        cJSON *root = cJSON_CreateObject();
        
        // Power state
        if (ac_conf[CONF_AC_POWER].value == 0) {
            cJSON_AddStringToObject(root, "mode", "off");
        } else {
            // Mode mapping
            switch ((int)ac_conf[CONF_AC_MODE].value) {
                case 0: cJSON_AddStringToObject(root, "mode", "auto"); break;
                case 1: cJSON_AddStringToObject(root, "mode", "cool"); break;
                case 2: cJSON_AddStringToObject(root, "mode", "heat"); break;
                case 3: cJSON_AddStringToObject(root, "mode", "dry"); break;
                case 4: cJSON_AddStringToObject(root, "mode", "fan_only"); break;
                default: cJSON_AddStringToObject(root, "mode", "auto"); break;
            }
        }
        
        // Temperature
        cJSON_AddNumberToObject(root, "temperature", ac_conf[CONF_AC_TEMPERATURE].value);
        
        // Fan mode
        switch ((int)ac_conf[CONF_AC_FAN_SPEED].value) {
            case 0: cJSON_AddStringToObject(root, "fan_mode", "auto"); break;
            case 1: cJSON_AddStringToObject(root, "fan_mode", "low"); break;
            case 2: cJSON_AddStringToObject(root, "fan_mode", "medium"); break;
            case 3: cJSON_AddStringToObject(root, "fan_mode", "high"); break;
            default: cJSON_AddStringToObject(root, "fan_mode", "auto"); break;
        }
        
        char *json = cJSON_PrintUnformatted(root);
        
        mqtt_lock();
        const char *topic = mqtt_state_topic ? mqtt_state_topic : "irbaby/state";
        // Make a copy to avoid potential race condition
        char *topic_copy = strdup(topic);
        mqtt_unlock();
        
        if (topic_copy) {
            esp_mqtt_client_publish(mqtt_client, topic_copy, json, 0, 1, 0);
            free(topic_copy);
        }
        free(json);
        cJSON_Delete(root);
    }
}

void mqtt_deinit(void)
{
    if (mqtt_client != NULL) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
    }
    
    mqtt_lock();
    free(mqtt_topic_prefix);
    free(mqtt_command_topic);
    free(mqtt_state_topic);
    free(mqtt_response_topic);
    free(mqtt_ha_discovery_topic);
    mqtt_topic_prefix = NULL;
    mqtt_command_topic = NULL;
    mqtt_state_topic = NULL;
    mqtt_response_topic = NULL;
    mqtt_ha_discovery_topic = NULL;
    mqtt_unlock();
    
    ESP_LOGI(TAG, "MQTT client deinitialized");
}