
set(COMPONENT_SRCS  "ICE40DirectConfig_WebUI.c"
)
# Fix cmake build

idf_component_register( SRCS "${COMPONENT_SRCS}"
                        
                        EMBED_FILES "webpage/web_interface.html" "webpage/web_interface.css" "webpage/web_interface.js"

                        INCLUDE_DIRS "." 
                        PRIV_REQUIRES driver spiffs esp_http_server ICE40DirectConfig json nvs_flash)
