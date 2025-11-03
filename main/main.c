#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "wifi_manager.h"
#include "wifi-process.h"
#include <esp_wifi.h>
#include "modbus-tcp.h"
#include "modbus-rtu.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "app_events.h"

static const char *TAG = "APP_MAIN";

// Global Event Group để đồng bộ giữa WiFi và Modbus tasks
EventGroupHandle_t app_event_group = NULL;

/**
 * @brief Khởi tạo NVS Flash
 */
static esp_err_t init_nvs(void)
{
    ESP_LOGI(TAG, "Khởi tạo NVS Flash...");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS Flash cần xóa, đang xóa...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "✓ NVS Flash khởi tạo thành công");
    } else {
        ESP_LOGE(TAG, "✗ NVS Flash khởi tạo thất bại: %s", esp_err_to_name(ret));
    }
    
    return ret;
}

/**
 * @brief Khởi tạo Event Group
 */
static esp_err_t init_event_group(void)
{
    ESP_LOGI(TAG, "Khởi tạo Event Group...");
    
    app_event_group = xEventGroupCreate();
    if (app_event_group == NULL) {
        ESP_LOGE(TAG, "✗ Không thể tạo Event Group");
        return ESP_ERR_NO_MEM;
    }
    
    ESP_LOGI(TAG, "✓ Event Group khởi tạo thành công");
    return ESP_OK;
}

/**
 * @brief Khởi động WiFi Manager Task
 */
static esp_err_t start_wifi_task(void)
{
    ESP_LOGI(TAG, "Khởi động WiFi Manager Task...");
    
    BaseType_t ret = xTaskCreate(
        wifi_process_task,
        "wifi_process",
        8192,
        NULL,
        5,
        NULL
    );
    
    if (ret != pdPASS) {
        ESP_LOGE(TAG, "✗ Không thể tạo WiFi Task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "✓ WiFi Task đã khởi động");
    return ESP_OK;
}

/**
 * @brief Khởi động Modbus TCP Task
 */
static esp_err_t start_modbus_task(void)
{
    ESP_LOGI(TAG, "Khởi động Modbus TCP Task...");
    
    esp_err_t ret = modbus_tcp_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "✗ Không thể khởi động Modbus TCP: %s", esp_err_to_name(ret));
        return ret;
    }
    
    ESP_LOGI(TAG, "✓ Modbus TCP Task đã khởi động");
    return ESP_OK;
}

/**
 * @brief Hàm main của ứng dụng
 * 
 * Luồng hoạt động:
 * 1. Khởi tạo NVS Flash
 * 2. Khởi tạo Event Group để đồng bộ
 * 3. Khởi động WiFi Manager Task
 * 4. Khởi động Modbus TCP Task
 * 
 * WiFi Task sẽ:
 * - Đọc config từ NVS
 * - Thử kết nối STA hoặc chuyển sang AP Mode
 * - Set event WIFI_STA_CONNECTED_BIT hoặc WIFI_AP_STARTED_BIT
 * 
 * Modbus Task sẽ:
 * - Chờ event từ WiFi (WIFI_STA_CONNECTED_BIT hoặc WIFI_AP_STARTED_BIT)
 * - Khởi động Modbus Slave với IP hiện tại
 * - Xử lý Modbus requests
 * - Dừng và thử lại nếu WiFi bị ngắt kết nối
 */
void app_main(void)
{
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   ESP32 Modbus TCP Gateway Starting    ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
    
    // Bước 1: Khởi tạo NVS Flash
    ESP_ERROR_CHECK(init_nvs());
    
    // Bước 2: Khởi tạo Event Group
    ESP_ERROR_CHECK(init_event_group());
    
    // Bước 3: Khởi động WiFi Manager Task
    ESP_ERROR_CHECK(start_wifi_task());
    
    // Bước 4: Khởi động Modbus TCP Task
    ESP_ERROR_CHECK(start_modbus_task());
    
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Hệ thống đã khởi động hoàn tất      ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
}