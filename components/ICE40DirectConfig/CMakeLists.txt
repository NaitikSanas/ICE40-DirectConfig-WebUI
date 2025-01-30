
set(COMPONENT_SRCS  "ICE40DirectConfig.c"
)
# Fix cmake build

idf_component_register(SRCS "${COMPONENT_SRCS}"
                        INCLUDE_DIRS "." 
                       PRIV_REQUIRES driver spiffs nvs_flash)
