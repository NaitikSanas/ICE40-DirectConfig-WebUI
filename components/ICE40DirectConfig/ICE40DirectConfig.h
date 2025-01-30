#include <stdio.h>
#include "esp_err.h"

#define FPGA_CS         CONFIG_FPGA_CS
#define FPGA_RESET      CONFIG_FPGA_RESET
#define FPGA_DONE       CONFIG_FPGA_DONE
#define FPGA_MOSI       CONFIG_FPGA_MOSI
#define FPGA_SCK        CONFIG_FPGA_SCK
#define SPI_HOST        CONFIG_SPI_HOST
#define SPI_CLOCK_SPEED CONFIG_SPI_CLOCK_SPEED
#define CHUNK_SIZE      CONFIG_CHUNK_SIZE

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