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

static const char *TAG = "wifi-process";
static bool wifi_initialized = false;

/**
 * @brief Callback function that gets called when WiFi successfully connects and receives IP
 */
void cb_connection_ok(void *pvParameter){
    ip_event_got_ip_t* param = (ip_event_got_ip_t*)pvParameter;
   
    /* transform IP to human readable string */
    char str_ip[16];
    esp_ip4addr_ntoa(&param->ip_info.ip, str_ip, IP4ADDR_STRLEN_MAX);
   
    ESP_LOGI(TAG, "I have a connection and my IP is %s!", str_ip);
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
        /* start the wifi manager only once */
        wifi_manager_start();
        
        /* register callback for IP acquisition event */
        wifi_manager_set_callback(WM_EVENT_STA_GOT_IP, &cb_connection_ok);
        
        wifi_initialized = true;
        ESP_LOGI(TAG, "WiFi manager started");
    }

    // Task main loop - required for FreeRTOS tasks
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}