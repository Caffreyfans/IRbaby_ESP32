set(main_srcs
    conf.c
    ir.c
    main.c)

set(main_priv_requires
    IRbaby
    app_update
    driver
    esp_system
    nvs_flash
    spiffs
    spi_flash)

set(embed_txtfiles)
set(wifi_targets esp32 esp32c2 esp32c3 esp32c5 esp32c6 esp32s2 esp32s3)
if(IDF_TARGET IN_LIST wifi_targets)
    set(target_has_wifi ON)
else()
    set(target_has_wifi OFF)
endif()

if(target_has_wifi)
    list(APPEND main_srcs
        dns_server.c
        form_parser.c
        homekit.c
        mqtt.c
        ota.c
        web.c
        web_handler.c
        wifimanager.c)
    list(APPEND main_priv_requires
        esp_event
        esp_http_client
        esp_http_server
        esp_netif
        esp_psram
        esp_wifi
        mqtt)
    list(APPEND embed_txtfiles
        front/config.json
        front/wificonfig.html
        front/root.html)
endif()

idf_component_register(SRCS ${main_srcs}
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES ${main_priv_requires}
                    EMBED_TXTFILES ${embed_txtfiles})
