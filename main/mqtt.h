/*
 * MQTT client header for IRbaby
 */
#ifndef MQTT_H_
#define MQTT_H_

#include "esp_err.h"
#include "mqtt_client.h"

extern esp_mqtt_client_handle_t mqtt_client;

esp_err_t mqtt_init(const char *broker_url, int port, const char *username, const char *password, const char *client_id, const char *topic_prefix);
void mqtt_deinit(void);
esp_err_t mqtt_publish_status(void);

#endif // MQTT_H_