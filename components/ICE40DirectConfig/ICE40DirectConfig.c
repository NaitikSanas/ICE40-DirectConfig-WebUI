#include "ICE40DirectConfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_spiffs.h"
#include "esp_log.h"

#define TAG "ICE40UP5K_LOADER"


// Function to initialize GPIO
void init_gpio() {
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << FPGA_CS) | (1ULL << FPGA_RESET),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    gpio_config(&io_conf);
    gpio_set_direction(FPGA_DONE, GPIO_MODE_INPUT);

    // Set default state
    gpio_set_level(FPGA_CS, 1);
    gpio_set_level(FPGA_RESET, 1);
}

// Function to send dummy clocks
void send_dummy_clocks(spi_device_handle_t spi) {
    uint8_t dummy = 0xFF;  // 8 bits of dummy data
    spi_transaction_t t = {
        .length = 8 * 8,  // 8 dummy clocks
        .tx_buffer = &dummy
    };
    spi_device_transmit(spi, &t);
}
static uint8_t buffer[CHUNK_SIZE];
// Function to configure the FPGA
esp_err_t configure_fpga(spi_device_handle_t spi, const char *file_name) {
    esp_err_t ret;
    
    // Step 1: Toggle CRESET_B and SPI_SS
    gpio_set_level(FPGA_RESET, 0);
    ets_delay_us(2); // Wait for >200 ns
    gpio_set_level(FPGA_RESET, 1);
    gpio_set_level(FPGA_CS, 0);

    // Step 2: Wait 1200 µs and send dummy clocks
    ets_delay_us(1200);
    // vTaskDelay(pdMS_TO_TICKS(2));  // Wait for >1200 µs
    gpio_set_level(FPGA_CS, 1);
    send_dummy_clocks(spi);
    gpio_set_level(FPGA_CS, 0);

    int64_t start_time = esp_timer_get_time();
    // Step 3: Open the bitstream file
    FILE *file = fopen(file_name, "rb");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open bitstream file: %s", file_name);
        return ESP_FAIL;
    }
    else {
        ESP_LOGI(TAG, "Opened bitstream file: %s", file_name);
    }

    // Step 4: Read and send the bitstream in chunks
    
    size_t bytes_read;
    size_t total_bytes_read  = 0;
    while ((bytes_read = fread(buffer, 1, CHUNK_SIZE, file)) > 0) {
        spi_transaction_t t = {
            .length = bytes_read * 8,
            .tx_buffer = buffer
        };
        ret = spi_device_transmit(spi, &t);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "SPI transmit failed");
            fclose(file);
            return ret;
        }
        total_bytes_read += bytes_read;
    }
    ESP_LOGI(TAG,"Total bytes sent %d\r\n",total_bytes_read);
    fclose(file);

    // Step 5: Wait for 100 additional clocks
    uint8_t clock_padding = 0xFF;
    for (int i = 0; i < 13; i++) {  // 13 bytes ~ 100 bits
        spi_transaction_t t = {
            .length = 8,
            .tx_buffer = &clock_padding
        };
        spi_device_transmit(spi, &t);
    }
    ESP_LOGI(TAG,"File written in %lld ms\r\n",(esp_timer_get_time() - start_time)/1000);
    // vTaskDelay(1);
    // Step 6: Check CDONE
    int retry = 200;
    while (gpio_get_level(FPGA_DONE) != 1 && retry > 0)
    {
        retry--;
        // vTaskDelay(1)
    }
    
    if (gpio_get_level(FPGA_DONE) == 1) {
        ESP_LOGI(TAG, "FPGA configuration successful!");
    } else {
        ESP_LOGE(TAG, "FPGA configuration failed!");
        return ESP_FAIL;
    }

    // Step 7: Wait 49 cycles for I/O to become active
    vTaskDelay(pdMS_TO_TICKS(1));  // Approximate delay for 49 cycles
    return ESP_OK;
}

// SPIFFS Initialization
esp_err_t init_spiffs() {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 20,
        .format_if_mount_failed = true
    };
    return esp_vfs_spiffs_register(&conf);
}
spi_device_handle_t spi;
void ICE40DirectConfig_Init(){
    // Initialize GPIO
    init_gpio();

    // // // Initialize SPIFFS
    if (init_spiffs() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPIFFS");
        return;
    }
    ESP_LOGI(TAG, "SPIFFS initialized");  

    // SPI Configuration
    spi_bus_config_t buscfg = {
        .mosi_io_num = GPIO_NUM_39,
        .miso_io_num = -1,
        .sclk_io_num = GPIO_NUM_37,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CHUNK_SIZE
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = SPI_CLOCK_SPEED,
        .mode = 0,
        .spics_io_num = -1,
        .queue_size = 1
    };

    
    if (spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO) == ESP_OK &&
        spi_bus_add_device(SPI_HOST, &devcfg, &spi) == ESP_OK) {
        ESP_LOGI(TAG, "Initialize SPI Done..");    
        } 
    else {
        ESP_LOGE(TAG, "Failed to initialize SPI");
        return ;
    }
}

#define STORAGE_NAMESPACE "storage"
#include "nvs_flash.h"
esp_err_t ICE40DirectConfig_Set_Active_Conf_Name(const char *key, const char *file_name) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Open NVS handle in read/write mode
    err = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    // Write the file name to NVS
    err = nvs_set_str(nvs_handle, key, file_name);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write file name to NVS: %s", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    // Commit changes to NVS
    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit changes to NVS: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "File name successfully written to NVS with key '%s'", key);
    }

    // Close NVS handle
    nvs_close(nvs_handle);

    return err;
}

esp_err_t ICE40DirectConfig_Get_Active_Conf_Name(const char *key, char *file_name, size_t max_len) {
    nvs_handle_t nvs_handle;
    esp_err_t err;

    // Initialize NVS if not already initialized
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "Erasing NVS flash...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize NVS: %s", esp_err_to_name(err));
        return err;
    }

    // Open NVS handle in read-only mode
    err = nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    // Read the string from NVS
    size_t required_len = 0;
    err = nvs_get_str(nvs_handle, key, NULL, &required_len);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGW(TAG, "Key '%s' not found in NVS.", key);
        nvs_close(nvs_handle);
        return err;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get string length for key '%s': %s", key, esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    if (required_len > max_len) {
        ESP_LOGE(TAG, "Buffer size (%zu) too small. Required size: %zu", max_len, required_len);
        nvs_close(nvs_handle);
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_str(nvs_handle, key, file_name, &required_len);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "File name read from NVS: %s", file_name);
    } else {
        ESP_LOGE(TAG, "Failed to read file name: %s", esp_err_to_name(err));
    }

    // Close NVS handle
    nvs_close(nvs_handle);

    return err;
}


int ICE40DirectConfig_Load_BitStream(char* file_name){
    char bitstream_file[64];
    int64_t start_time = esp_timer_get_time();
    sprintf(bitstream_file,"/spiffs/%s",file_name);
    if (configure_fpga(spi, bitstream_file) != ESP_OK) {
        return 0;
    }
    else {
        ESP_LOGI(TAG,"Configured FPGA in %lld ms\r\n",(esp_timer_get_time() - start_time)/1000);
        return 1;
    }   
}

int ICE40DirectConfig_Load_Active_BitStream(void){
    char active_config_file_name [32];
    if(ESP_OK == ICE40DirectConfig_Get_Active_Conf_Name("active_config",active_config_file_name,32)){
        ESP_LOGI(TAG,"active_config file %s\r\n",active_config_file_name);
        if(ICE40DirectConfig_Load_BitStream(active_config_file_name)){
            return 1;
        }
        else {
            ESP_LOGE(TAG,"Active App File Unavailable On Storage");
            ICE40DirectConfig_Set_Active_Conf_Name("active_config"," ");
            return 0;
        }
    }
    return 0;
}
 