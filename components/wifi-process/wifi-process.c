#include "wifi-process.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_err.h"
#include "wifi_manager.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "app_events.h"

static const char *TAG = "WIFI_PROCESS";
static bool wifi_initialized = false;

// Import global event group từ main
extern EventGroupHandle_t app_event_group;

/**
 * @brief Callback function that gets called when WiFi successfully connects and receives IP
 */
void cb_connection_ok(void *pvParameter){
    ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;
   
    /* transform IP to human readable string */
    char str_ip[16];
    esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
   
    ESP_LOGI(TAG, "✓ WiFi STA kết nối thành công! IP: %s", str_ip);
    
    // Set event để báo cho Modbus Task biết WiFi đã kết nối
    if (app_event_group != NULL) {
        xEventGroupClearBits(app_event_group, WIFI_DISCONNECTED_BIT);
        xEventGroupSetBits(app_event_group, WIFI_STA_CONNECTED_BIT);
        ESP_LOGI(TAG, "→ Event WIFI_STA_CONNECTED_BIT đã được set");
    }
}

/**
 * @brief Callback function khi WiFi STA bị ngắt kết nối
 */
void cb_connection_lost(void *pvParameter){
    static uint8_t disconnect_count = 0;
    disconnect_count++;
    
    ESP_LOGW(TAG, "✗ WiFi STA bị ngắt kết nối (lần %d)", disconnect_count);
    ESP_LOGW(TAG, "→ WiFi Manager sẽ retry tối đa 3 lần trước khi start AP Mode");
    
    // Set event để báo cho Modbus Task biết WiFi bị ngắt
    if (app_event_group != NULL) {
        xEventGroupClearBits(app_event_group, WIFI_STA_CONNECTED_BIT);
        xEventGroupSetBits(app_event_group, WIFI_DISCONNECTED_BIT);
        ESP_LOGW(TAG, "→ Event WIFI_DISCONNECTED_BIT đã được set");
    }
}
        
/**
 * @brief Callback function khi AP Mode được khởi động
 */
void cb_ap_started(void *pvParameter){
    ESP_LOGI(TAG, "✓ WiFi AP Mode đã khởi động");
    
    // Set event để báo cho Modbus Task biết AP đã sẵn sàng
    if (app_event_group != NULL) {
        xEventGroupSetBits(app_event_group, WIFI_AP_STARTED_BIT);
        ESP_LOGI(TAG, "→ Event WIFI_AP_STARTED_BIT đã được set");
    }
}

void wifi_process_disconnect() {
    ESP_LOGI(TAG, "Disconnecting from WiFi...");
    wifi_manager_send_message(WM_ORDER_DISCONNECT_STA, NULL);
}

void wifi_process_connect() {
    ESP_LOGI(TAG, "Connecting to WiFi...");
    wifi_manager_connect_async();
}

void wifi_process_get_status() {
    wifi_config_t* config = wifi_manager_get_wifi_sta_config();
    if (config) {
        ESP_LOGI(TAG, "Current WiFi SSID: %s", config->sta.ssid);
    } else {
        ESP_LOGI(TAG, "No WiFi configuration found");
    }
}

void wifi_process_task(void *pvParameters){
    if (!wifi_initialized) {
        ESP_LOGI(TAG, "Đang khởi động WiFi Manager...");
        
        /* start the wifi manager only once */
        wifi_manager_start();
        
        /* register callbacks cho các WiFi events */
        wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
        wifi_manager_set_callback(WM_EVENT_STA_DISCONNECTED, &cb_connection_lost);
        wifi_manager_set_callback(WM_ORDER_START_AP, &cb_ap_started);
        
        wifi_initialized = true;
        ESP_LOGI(TAG, "✓ WiFi Manager đã khởi động");
        ESP_LOGI(TAG, "→ Đang đọc config từ NVS và thử kết nối...");
    }

    // Task main loop - required for FreeRTOS tasks
    while(1) {
        // Giữ task alive, WiFi Manager sẽ xử lý các events qua callbacks
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}