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

static const char *TAG = "mqtt";

esp_mqtt_client_handle_t mqtt_client = NULL;
static char *mqtt_topic_prefix = NULL;
static char *mqtt_command_topic = NULL;
static char *mqtt_state_topic = NULL;
static char *mqtt_response_topic = NULL;

static void mqtt_build_topics(const char *prefix)
{
    const char *base = (prefix && prefix[0]) ? prefix : "irbaby";
    free(mqtt_topic_prefix);
    free(mqtt_command_topic);
    free(mqtt_state_topic);
    free(mqtt_response_topic);

    mqtt_topic_prefix = strdup(base);
    if (!mqtt_topic_prefix) {
        mqtt_topic_prefix = strdup("irbaby");
    }

    int len = snprintf(NULL, 0, "%s/command", mqtt_topic_prefix) + 1;
    mqtt_command_topic = malloc(len);
    snprintf(mqtt_command_topic, len, "%s/command", mqtt_topic_prefix);

    len = snprintf(NULL, 0, "%s/state", mqtt_topic_prefix) + 1;
    mqtt_state_topic = malloc(len);
    snprintf(mqtt_state_topic, len, "%s/state", mqtt_topic_prefix);

    len = snprintf(NULL, 0, "%s/response", mqtt_topic_prefix) + 1;
    mqtt_response_topic = malloc(len);
    snprintf(mqtt_response_topic, len, "%s/response", mqtt_topic_prefix);
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
        if (mqtt_state_topic) {
            msg_id = esp_mqtt_client_publish(client, mqtt_state_topic, "online", 0, 1, 0);
            ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        }

        if (mqtt_command_topic) {
            msg_id = esp_mqtt_client_subscribe(client, mqtt_command_topic, 0);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        }
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

void mqtt_init(const char *broker_url, int port, const char *username, const char *password, const char *client_id, const char *topic_prefix)
{
    if (mqtt_client != NULL) {
        esp_mqtt_client_stop(mqtt_client);
        esp_mqtt_client_destroy(mqtt_client);
        mqtt_client = NULL;
    }

    if (!broker_url || strlen(broker_url) == 0) {
        ESP_LOGE(TAG, "MQTT broker URL is empty");
        return;
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
        return;
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
    } else {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
    }
}

void mqtt_publish_status()
{
    if (mqtt_client != NULL) {
        property_t *ac_conf = irbaby_get_conf(CONF_AC);
        cJSON *root = cJSON_CreateObject();
        cJSON_AddNumberToObject(root, "power", ac_conf[CONF_AC_POWER].value);
        cJSON_AddNumberToObject(root, "mode", ac_conf[CONF_AC_MODE].value);
        cJSON_AddNumberToObject(root, "temp", ac_conf[CONF_AC_TEMPERATURE].value);
        cJSON_AddNumberToObject(root, "fan_speed", ac_conf[CONF_AC_FAN_SPEED].value);
        char *json = cJSON_PrintUnformatted(root);
        const char *topic = mqtt_state_topic ? mqtt_state_topic : "irbaby/state";
        esp_mqtt_client_publish(mqtt_client, topic, json, 0, 1, 0);
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
    free(mqtt_topic_prefix);
    free(mqtt_command_topic);
    free(mqtt_state_topic);
    free(mqtt_response_topic);
    mqtt_topic_prefix = NULL;
    mqtt_command_topic = NULL;
    mqtt_state_topic = NULL;
    mqtt_response_topic = NULL;
    ESP_LOGI(TAG, "MQTT client deinitialized");
}