file(GLOB srcs "*.c")
idf_component_register(SRCS ${srcs} 
                    INCLUDE_DIRS "."
                    EMBED_TXTFILES front/config.json front/wificonfig.html front/root.html front/irext.js)
