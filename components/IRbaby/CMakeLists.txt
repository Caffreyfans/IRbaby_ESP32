
option(ESP32 "build ESP32?" ON)


file(GLOB irext_srcs "IRbaby_SDK/3rdparty/IRext/*.c")
set(srcs "IRbaby_SDK/3rdparty/coreHTTP/coreHTTP/source/dependency/3rdparty/http_parser/http_parser.c"
        "IRbaby_SDK/3rdparty/coreHTTP/coreHTTP/source/core_http_client"
        "IRbaby_SDK/3rdparty/coreHTTP/port/http_client.c"
        "IRbaby_SDK/3rdparty/cJSON/cJSON/cJSON.c"
        "IRbaby_SDK/src/port/peripherals.c"
        "IRbaby_SDK/src/port/storage.c"
        "IRbaby_SDK/src/irext_api.c"
        "IRbaby_SDK/src/irbaby.c")
set(includes "IRbaby_SDK/3rdparty/coreHTTP/coreHTTP/source/dependency/3rdparty/http_parser"
             "IRbaby_SDK/3rdparty/coreHTTP/coreHTTP/source/include"
             "IRbaby_SDK/3rdparty/coreHTTP/coreHTTP/source/interface"
             "IRbaby_SDK/3rdparty/coreHTTP/port/include"
             "IRbaby_SDK/3rdparty/cJSON/cJSON"
             "IRbaby_SDK/3rdparty/IRext/include"
             "IRbaby_SDK/include"
             "IRbaby_SDK/include/port")
idf_component_register(SRCS ${irext_srcs} ${srcs}
                       INCLUDE_DIRS ${includes}
                       REQUIRES lwip freertos driver esp_timer)

target_compile_definitions(${COMPONENT_LIB} PUBLIC -DHTTP_DO_NOT_USE_CUSTOM_CONFIG=1 -DESP32)
target_compile_options(${COMPONENT_LIB} PUBLIC -Wno-stringop-truncation)
