/*
 * HomeKit header for IRbaby ESP32
 */
#ifndef HOMEKIT_H_
#define HOMEKIT_H_

#include "esp_err.h"
#include <stdbool.h>

// HomeKit configuration operations (defined in conf.h)
// typedef enum {
//     CONF_HOMEKIT_ENABLE = 0,
//     CONF_HOMEKIT_SETUP_CODE,
//     CONF_HOMEKIT_MAX
// } homekit_ops_t;

// Initialize HomeKit accessory bridge
esp_err_t homekit_init(void);

// Deinitialize HomeKit
esp_err_t homekit_deinit(void);

// Enable/disable HomeKit
esp_err_t homekit_set_enabled(bool enabled);

// Check if HomeKit is enabled
bool homekit_is_enabled(void);

// Get HomeKit setup code
const char* homekit_get_setup_code(void);

// Set HomeKit setup code
esp_err_t homekit_set_setup_code(const char *setup_code);

// Get HomeKit accessory information (JSON for web interface)
char* homekit_get_accessory_info(void);

#endif // HOMEKIT_H_