
#include "ICE40DirectConfig_WebUI.h"
#include "math.h"
#include "ICE40DirectConfig.h"

static const char *TAG = "HTTP_SERVER";

extern const uint8_t web_interface_html_start[]				asm("_binary_web_interface_html_start");
extern const uint8_t web_interface_html_end[]				asm("_binary_web_interface_html_end");

extern const uint8_t web_interface_css_start[]				asm("_binary_web_interface_css_start");
extern const uint8_t web_interface_css_end[]				asm("_binary_web_interface_css_end");

extern const uint8_t web_interface_js_start[]				asm("_binary_web_interface_js_start");
extern const uint8_t web_interface_js_end[]				    asm("_binary_web_interface_js_end");

// Handler for serving HTML
esp_err_t html_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving HTML file");
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)web_interface_html_start, web_interface_html_end - web_interface_html_start);
    return ESP_OK;
}

// Handler for serving CSS
esp_err_t css_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving CSS file");
    httpd_resp_set_type(req, "text/css");
    httpd_resp_send(req, (const char *)web_interface_css_start, web_interface_css_end - web_interface_css_start);
    return ESP_OK;
}

// Handler for serving JavaScript
esp_err_t js_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG, "Serving JavaScript file");
    httpd_resp_set_type(req, "application/javascript");
    httpd_resp_send(req, (const char *)web_interface_js_start, web_interface_js_end - web_interface_js_start);
    return ESP_OK;
}


#define FILE_PATH_MAX 128
#define BUFFER_SIZE 1024



// Helper function to save data to SPIFFS
static esp_err_t save_to_spiffs(const char *filename, const char *data, size_t len, bool append) {
    const char *mode = append ? "a" : "w";
    FILE *file = fopen(filename, mode);
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file %s for writi ng", filename);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, file);
    fclose(file);

    if (written != len) {
        ESP_LOGE(TAG, "Failed to write complete data to file");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "File %s written successfully", filename);
    return ESP_OK;
}

esp_err_t file_upload_handler(httpd_req_t *req) {
    char filepath[64] = "/spiffs/";
    char buf[512];
    int remaining = req->content_len;
    int received;
    bool header_parsed = false;

    // Buffer to store the filename
    char filename[32] = {0};

    // Buffer to store the full headers
    char header_buf[512] = {0};
    int header_buf_len = 0;

    // Open a temporary file for writing
    FILE *fd = NULL;

    while (remaining > 0) {
        received = httpd_req_recv(req, buf, fmin(remaining, sizeof(buf)));
        if (received <= 0) {
            ESP_LOGE(TAG, "File reception failed");
            if (fd) fclose(fd);
            return ESP_FAIL;
        }

        remaining -= received;

        // If headers haven't been parsed yet, buffer them until \r\n\r\n
        if (!header_parsed) {
            int copy_len = fmin(received, sizeof(header_buf) - header_buf_len - 1);
            memcpy(header_buf + header_buf_len, buf, copy_len);
            header_buf_len += copy_len;
            header_buf[header_buf_len] = '\0';

            // Look for end of headers
            char *file_start = strstr(header_buf, "\r\n\r\n");
            if (file_start) {
                file_start += 4; // Move past the header delimiter
                header_parsed = true;

                // Extract the filename
                char *start = strstr(header_buf, "filename=\"");
                if (start) {
                    start += 10; // Move past 'filename="'
                    char *end = strchr(start, '"');
                    if (end) {
                        size_t len = end - start;
                        len = len < sizeof(filename) - 1 ? len : sizeof(filename) - 1;
                        strncpy(filename, start, len);
                        filename[len] = '\0';
                    }
                }

                // If no filename found, use a default
                if (filename[0] == '\0') {
                    strcpy(filename, "uploaded_file.bin");
                }

                // Build the file path
                strcat(filepath, filename);

                ESP_LOGI(TAG, "Saving file as: %s", filepath);

                // Open the file for writing
                fd = fopen(filepath, "w");
                if (!fd) {
                    ESP_LOGE(TAG, "Failed to open file: %s", filepath);
                    return ESP_FAIL;
                }

                // Write the first chunk of file data (after headers) to the file
                int header_len = file_start - header_buf;
                fwrite(file_start, 1, received - header_len, fd);
            }
        } else {
            // Write the rest of the file data to SPIFFS
            if (fd) fwrite(buf, 1, received, fd);
        }
        
    }

    if (fd) fclose(fd);

    if (!header_parsed) {
        ESP_LOGE(TAG, "Failed to parse file data");
        return ESP_FAIL;
    }
    

    httpd_resp_sendstr(req, "File uploaded successfully");
    // ICE40DirectConfig_Load_BitStream(filename);
    ESP_LOGI(TAG, "File successfully saved: %s", filepath);
    return ESP_OK;
}
#include "dirent.h"
// esp_err_t list_files_in_spiffs(char *output, size_t max_len) {
//     const char *base_path = "/spiffs";
//     DIR *dir = opendir(base_path);
//     if (!dir) {
//         ESP_LOGE(TAG, "Failed to open directory: %s", strerror(errno));
//         return ESP_FAIL;
//     }

//     struct dirent *entry;
//     size_t offset = 0;

//     while ((entry = readdir(dir)) != NULL) {
//         if (entry->d_type == DT_REG) { // Regular file
//             snprintf(output + offset, max_len - offset, "%s\n", entry->d_name);
//             offset += strlen(output + offset);
//             if (offset >= max_len) break;
//         }
//     }
//     closedir(dir);

//     if (offset == 0) {
//         snprintf(output, max_len, "No files found\n");
//     }
//     return ESP_OK;
// }


esp_err_t list_bin_files_in_spiffs(char *output, size_t max_len) {
    const char *base_path = "/spiffs";
    DIR *dir = opendir(base_path);
    if (!dir) {
        ESP_LOGE(TAG, "Failed to open directory: %s", strerror(errno));
        return ESP_FAIL;
    }
    char active_file_name [32];
    if(ESP_OK == ICE40DirectConfig_Get_Active_Conf_Name("active_config",active_file_name,32)){

    }
    struct dirent *entry;
    size_t offset = 0;
    int index = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) { // Regular file
            const char *filename = entry->d_name;
            const char *ext = strrchr(filename, '.'); // Find the last occurrence of '.'
            if (ext && strcmp(ext, ".bin") == 0) {    // Check if the extension is ".bin"
                if(!strncmp(filename,active_file_name,strlen(active_file_name))){
                   snprintf(output + offset, max_len - offset, "%s", "(Active):"); 
                   offset += strlen(output + offset);
                }
                

                snprintf(output + offset, max_len - offset, "%s\n", filename);
                
                offset += strlen(output + offset);
                if (offset >= max_len) break;
                index++;
            }
        }
    }
    closedir(dir);

    if (offset == 0) {
        snprintf(output, max_len, "No .bin files found\n");
    }
    return ESP_OK;
}

// HTTP GET handler for /list_files
esp_err_t list_files_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Handling /list_files");

    const size_t buffer_size = 1024;
    char *file_list = malloc(buffer_size);
    if (!file_list) {
        ESP_LOGE(TAG, "Failed to allocate memory for file list");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_ERR_NO_MEM;
    }

    esp_err_t ret = list_bin_files_in_spiffs(file_list, buffer_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to list files");
        free(file_list);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to list files");
        return ret;
    }

    // Send the response
    ESP_LOGI(TAG,"File list %s\r\n",file_list);
    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, file_list);

    free(file_list);
    return ESP_OK;
}
#include "cJSON.h"

esp_err_t fpga_config_handler(httpd_req_t *req) {
    char buf[512];
    int ret, remaining = req->content_len;

    // Read the incoming JSON payload
    ret = httpd_req_recv(req, buf, fmin(remaining, sizeof(buf) - 1));
    if (ret <= 0) {
        ESP_LOGE(TAG, "Failed to receive data");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive data");
        return ESP_FAIL;
    }
    buf[ret] = '\0'; // Null-terminate the buffer

    ESP_LOGI(TAG, "Received payload: %s", buf);

    // Parse JSON to extract the file name
    cJSON *json = cJSON_Parse(buf);
    if (!json) {
        ESP_LOGE(TAG, "Invalid JSON received: %s", buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    cJSON *file_name_item = cJSON_GetObjectItem(json, "file_name");
    if (!file_name_item || !cJSON_IsString(file_name_item)) {
        ESP_LOGE(TAG, "File name not provided or invalid in JSON: %s", buf);
        cJSON_Delete(json);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File name missing or invalid");
        return ESP_FAIL;
    }

    const char *file_name = file_name_item->valuestring;
    ESP_LOGI(TAG, "Received FPGA configuration file: %s", file_name);
    char filepath[64];
    snprintf(filepath, sizeof(filepath), "/spiffs/%s", file_name);
    // Check if the file exists
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        ESP_LOGE(TAG, "File not found: %s", filepath);
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }
    fclose(f);

    // Proceed with FPGA configuration
    ESP_LOGI(TAG, "Configuring FPGA with file: %s", filepath);
    ICE40DirectConfig_Set_Active_Conf_Name("active_config",file_name);
    ICE40DirectConfig_Load_BitStream(file_name);
    httpd_resp_sendstr(req, "FPGA_CONFIG_SUCCESS!");
    cJSON_Delete(json);
    return ESP_OK;
}


// Function to start the HTTP server
void ICE40DirectConfig_WebUI_Init(void)
{
    
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 10000;
    config.task_priority = 2;
    // Start the HTTP server
    ESP_LOGI(TAG, "Starting server on port: %d", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK)
    {
        // Register URI handlers
        httpd_uri_t html_uri = {
            .uri = "/web_interface.html",
            .method = HTTP_GET,
            .handler = html_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &html_uri);

        httpd_uri_t css_uri = {
            .uri = "/web_interface.css",
            .method = HTTP_GET,
            .handler = css_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &css_uri);

        httpd_uri_t js_uri = {
            .uri = "/web_interface.js",
            .method = HTTP_GET,
            .handler = js_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &js_uri);

        httpd_uri_t file_upload_uri = {
            .uri = "/file_upload",
            .method = HTTP_POST,
            .handler = file_upload_handler,
            .user_ctx = NULL};
        httpd_register_uri_handler(server, &file_upload_uri);

        httpd_uri_t list_files_uri = {
            .uri       = "/list_files",
            .method    = HTTP_GET,
            .handler   = list_files_handler,
            .user_ctx  = NULL
        };
        httpd_register_uri_handler(server, &list_files_uri);

        httpd_uri_t fpga_config_uri = {
            .uri       = "/fpga_config",
            .method    = HTTP_POST,
            .handler   = fpga_config_handler,
            .user_ctx  = NULL
        };

        // Register the URI
        if (httpd_register_uri_handler(server, &fpga_config_uri) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to register /fpga_config URI");
            return ESP_FAIL;
        }
        ESP_LOGI(TAG, "HTTP server started.");

    }
    else
    {
        ESP_LOGE(TAG, "Failed to start HTTP server.");
    }
    
}