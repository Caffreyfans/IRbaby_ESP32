file(GLOB srcs "*.c")
idf_component_register(SRCS ${srcs} 
                    INCLUDE_DIRS "."
                    REQUIRES esp_psram IRbaby esp_netif esp_http_server esp_wifi nvs_flash spiffs spi_flash newlib esp_system driver
                    EMBED_TXTFILES front/config.json front/wificonfig.html front/root.html)
