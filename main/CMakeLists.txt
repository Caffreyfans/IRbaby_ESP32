file(GLOB srcs "*.c")
idf_component_register(SRCS ${srcs} ota.c mqtt.c homekit.c
                    INCLUDE_DIRS "."
                    REQUIRES esp_psram IRbaby esp_netif esp_http_server esp_wifi nvs_flash spiffs spi_flash newlib esp_system driver esp_http_client app_update mqtt
                    EMBED_TXTFILES front/config.json front/wificonfig.html front/root.html)
