#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "ICE40DirectConfig.h"
#include <stdio.h>
#include <string.h>
#include "wifi_manager.h"
#define WIFI_CREDENTIALS "AP_SSID", "AP_PASSWORD"


#include "ICE40DirectConfig_WebUI.h"

#define TAG "MAIN_APP"
void app_main(void) {
    wifi_init();
    wifi_connect(WIFI_CREDENTIALS);
    
    ICE40DirectConfig_WebUI_Init();
    ICE40DirectConfig_Init();
    ICE40DirectConfig_Load_Active_BitStream();
}
