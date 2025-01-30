#include <stdio.h>
#include "esp_err.h"

// GPIO Pins
#define FPGA_CS       GPIO_NUM_40   // SPI_SS
#define FPGA_RESET    GPIO_NUM_41   // CRESET_B
#define FPGA_DONE     GPIO_NUM_38  // CDONE (optional)
#define FPGA_MOSI     GPIO_NUM_39
#define FPGA_SCK      GPIO_NUM_37
// SPI Configuration
#define SPI_HOST          SPI2_HOST
#define SPI_CLOCK_SPEED   100000000  // 8 MHz
#define CHUNK_SIZE        20*1024     // Chunk size (1 KB)

/**
 * @brief Initialized SPI BUS and SPIFFS to Load File into FPGA
 */
void        ICE40DirectConfig_Init();

/**
 * @brief Loads Bitstream file into FPGA By pointing at any file by name in SPIFFS or SDCard
 */
int         ICE40DirectConfig_Load_BitStream(char* file_name);


/**
 * @brief Gets Active Config Bitstream file name from NVS
 */
esp_err_t   ICE40DirectConfig_Get_Active_Conf_Name(const char *key, char *file_name, size_t max_len);

/**
 * @brief Sets Active Config Bitstream file name from NVS
 */
esp_err_t   ICE40DirectConfig_Set_Active_Conf_Name(const char *key, const char *file_name) ;

/**
 * @brief Loads FPGA with Bitstream pointing at Active Conf file name.
 */
int         ICE40DirectConfig_Load_Active_BitStream(void);