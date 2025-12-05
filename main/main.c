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
    //ESP_ERROR_CHECK(start_modbus_task());
    
    ESP_LOGI(TAG, "╔════════════════════════════════════════╗");
    ESP_LOGI(TAG, "║   Hệ thống đã khởi động hoàn tất      ║");
    ESP_LOGI(TAG, "╚════════════════════════════════════════╝");
}