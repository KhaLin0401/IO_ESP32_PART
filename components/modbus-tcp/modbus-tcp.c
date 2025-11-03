/**
 * @file modbus-tcp.c
 * @brief Modbus TCP Slave implementation using ESP-Modbus library
 */

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"

#include "modbus-tcp-map.h"
#include "esp_modbus_slave.h"
#include "app_events.h"

// Tag
static const char *TAG = "MODBUS_TCP";

// Import global event group từ main
extern EventGroupHandle_t app_event_group;

// Task & control
static TaskHandle_t modbus_task_handle = NULL;
static EventGroupHandle_t modbus_event_group = NULL;
#define MODBUS_START_BIT    BIT0
#define MODBUS_STOP_BIT     BIT1
#define MODBUS_RUNNING_BIT  BIT2

// Modbus slave handle
static void *slave_handle = NULL;

// Config
#define MODBUS_TASK_STACK_SIZE    (4096)
#define MODBUS_TASK_PRIORITY      (5)
#define MODBUS_POLL_TIMEOUT_MS    (100)
#define MODBUS_UPDATE_INTERVAL_MS (1000)

// Forward declarations
static void modbus_task(void *pvParameters);
static esp_err_t modbus_slave_init_tcp(void);
static void modbus_update_input_registers(void);
static void modbus_update_discrete_inputs(void);

/* ==================================================================
 *  PUBLIC API
 * ================================================================== */

esp_err_t modbus_tcp_start(void)
{
    if (modbus_task_handle != NULL) {
        ESP_LOGW(TAG, "Modbus TCP task already running");
        return ESP_ERR_INVALID_STATE;
    }

    modbus_event_group = xEventGroupCreate();
    if (!modbus_event_group) {
        ESP_LOGE(TAG, "Failed to create event group");
        return ESP_ERR_NO_MEM;
    }

    BaseType_t ret = xTaskCreate(
        modbus_task,
        "modbus_tcp",
        MODBUS_TASK_STACK_SIZE,
        NULL,
        MODBUS_TASK_PRIORITY,
        &modbus_task_handle
    );

    if (ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create Modbus task");
        vEventGroupDelete(modbus_event_group);
        modbus_event_group = NULL;
        return ESP_ERR_NO_MEM;
    }

    xEventGroupSetBits(modbus_event_group, MODBUS_START_BIT);
    ESP_LOGI(TAG, "Modbus TCP task created and started");
    return ESP_OK;
}

esp_err_t modbus_tcp_stop(void)
{
    if (!modbus_task_handle) return ESP_OK;

    xEventGroupSetBits(modbus_event_group, MODBUS_STOP_BIT);

    // Wait up to 5s for task to finish
    vTaskDelay(pdMS_TO_TICKS(5000));

    if (modbus_task_handle != NULL) {
        ESP_LOGW(TAG, "Force delete Modbus task");
        vTaskDelete(modbus_task_handle);
        modbus_task_handle = NULL;
    }

    if (modbus_event_group) {
        vEventGroupDelete(modbus_event_group);
        modbus_event_group = NULL;
    }
    
    ESP_LOGI(TAG, "Modbus TCP task stopped");
    return ESP_OK;
}

/* ==================================================================
 *  MODBUS TASK
 * ================================================================== */

static void modbus_task(void *pvParameters)
{
    esp_err_t err;
    EventBits_t bits;
    EventBits_t wifi_bits;
    TickType_t last_update = 0;
    mb_event_group_t event;

    ESP_LOGI(TAG, "Modbus TCP task đã khởi động");
    ESP_LOGI(TAG, "→ Đang chờ WiFi kết nối (STA hoặc AP)...");

    while (1) {
        // Chờ START signal từ modbus_tcp_start()
        bits = xEventGroupWaitBits(
            modbus_event_group,
            MODBUS_START_BIT | MODBUS_STOP_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY
        );

        if (bits & MODBUS_STOP_BIT) break;
        if (!(bits & MODBUS_START_BIT)) continue;

        xEventGroupClearBits(modbus_event_group, MODBUS_START_BIT);

        // === BƯỚC 1: Chờ WiFi Connected (STA hoặc AP) ===
        ESP_LOGI(TAG, "Chờ event: WIFI_STA_CONNECTED hoặc WIFI_AP_STARTED...");
        
        wifi_bits = xEventGroupWaitBits(
            app_event_group,
            WIFI_STA_CONNECTED_BIT | WIFI_AP_STARTED_BIT,
            pdFALSE,  // Không clear bits
            pdFALSE,  // Chờ bất kỳ bit nào (OR)
            portMAX_DELAY
        );

        if (wifi_bits & WIFI_STA_CONNECTED_BIT) {
            ESP_LOGI(TAG, "✓ WiFi STA đã kết nối, đang khởi động Modbus TCP...");
        } else if (wifi_bits & WIFI_AP_STARTED_BIT) {
            ESP_LOGI(TAG, "✓ WiFi AP đã sẵn sàng, đang khởi động Modbus TCP...");
        }

        // === BƯỚC 2: Khởi tạo Modbus TCP Slave ===
        err = modbus_slave_init_tcp();
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "✗ Khởi tạo thất bại, thử lại sau 5s...");
            vTaskDelay(pdMS_TO_TICKS(5000));
            xEventGroupSetBits(modbus_event_group, MODBUS_START_BIT); // Retry
            continue;
        }

        xEventGroupSetBits(modbus_event_group, MODBUS_RUNNING_BIT);
        ESP_LOGI(TAG, "✓ Modbus TCP Slave đang chạy");

        // === BƯỚC 3: Main Loop - Poll Modbus & Monitor WiFi ===
        while (!(xEventGroupGetBits(modbus_event_group) & MODBUS_STOP_BIT)) {
            
            // Kiểm tra WiFi có bị ngắt kết nối không
            wifi_bits = xEventGroupGetBits(app_event_group);
            if (wifi_bits & WIFI_DISCONNECTED_BIT) {
                ESP_LOGW(TAG, "⚠ WiFi bị ngắt kết nối, dừng Modbus và chờ kết nối lại...");
                break; // Thoát loop để cleanup và restart
            }

            // Check for Modbus events
            event = mbc_slave_check_event(slave_handle, MB_EVENT_HOLDING_REG_WR | MB_EVENT_INPUT_REG_RD | 
                                          MB_EVENT_HOLDING_REG_RD | MB_EVENT_COILS_WR | 
                                          MB_EVENT_COILS_RD | MB_EVENT_DISCRETE_RD);
            
            if (event) {
                ESP_LOGD(TAG, "Modbus event: 0x%x", (int)event);
                
                // Handle specific events if needed
                if (event & MB_EVENT_HOLDING_REG_WR) {
                    ESP_LOGD(TAG, "Holding register được ghi");
                    // TODO: Handle configuration changes
                }
            }

            // Periodic update of input registers and discrete inputs
            TickType_t now = xTaskGetTickCount();
            if ((now - last_update) >= pdMS_TO_TICKS(MODBUS_UPDATE_INTERVAL_MS)) {
                modbus_update_input_registers();
                modbus_update_discrete_inputs();
                last_update = now;
            }

            vTaskDelay(pdMS_TO_TICKS(10));
        }

        // === BƯỚC 4: Cleanup ===
        ESP_LOGI(TAG, "Dừng Modbus TCP Slave...");
        mbc_slave_stop(slave_handle);
        mbc_slave_delete(slave_handle);
        slave_handle = NULL;
        xEventGroupClearBits(modbus_event_group, MODBUS_RUNNING_BIT);
        
        // Nếu WiFi bị ngắt, chờ kết nối lại
        wifi_bits = xEventGroupGetBits(app_event_group);
        if (wifi_bits & WIFI_DISCONNECTED_BIT) {
            xEventGroupSetBits(modbus_event_group, MODBUS_START_BIT); // Tự động retry
        }
    }

    ESP_LOGI(TAG, "Modbus task thoát");
    modbus_task_handle = NULL;
    vTaskDelete(NULL);
}

/* ==================================================================
 *  INIT
 * ================================================================== */

static esp_err_t modbus_slave_init_tcp(void)
{
    mb_communication_info_t comm_info = {
        .tcp_opts.port = MODBUS_TCP_PORT,
        .tcp_opts.mode = MB_TCP,
        .tcp_opts.addr_type = MB_IPV4,
        .tcp_opts.ip_addr_table = NULL,  // Bind to any address
        .tcp_opts.ip_netif_ptr = NULL,   // Use default netif
        .tcp_opts.uid = 1                // Slave address
    };

    esp_err_t err = mbc_slave_create_tcp(&comm_info, &slave_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_slave_create_tcp failed: %s", esp_err_to_name(err));
        return err;
    }

    mb_register_area_descriptor_t reg_area;

    // Register Holding Registers area
    reg_area.type = MB_PARAM_HOLDING;
    reg_area.start_offset = 0;  // Start from address 40001
    reg_area.address = (void*)&holding_reg_params;
    reg_area.size = sizeof(holding_reg_params_t);
    reg_area.access = MB_ACCESS_RW;
    err = mbc_slave_set_descriptor(slave_handle, reg_area);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_slave_set_descriptor HOLDING failed: %s", esp_err_to_name(err));
        mbc_slave_delete(slave_handle);
        slave_handle = NULL;
        return err;
    }

    // Register Input Registers area
    reg_area.type = MB_PARAM_INPUT;
    reg_area.start_offset = 0;  // Start from address 30001
    reg_area.address = (void*)&input_reg_params;
    reg_area.size = sizeof(input_reg_params_t);
    reg_area.access = MB_ACCESS_RW;
    err = mbc_slave_set_descriptor(slave_handle, reg_area);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_slave_set_descriptor INPUT failed: %s", esp_err_to_name(err));
        mbc_slave_delete(slave_handle);
        slave_handle = NULL;
        return err;
    }

    // Register Coils area
    reg_area.type = MB_PARAM_COIL;
    reg_area.start_offset = 0;  // Start from address 00001
    reg_area.address = (void*)&coil_reg_params;
    reg_area.size = sizeof(coil_reg_params_t);
    reg_area.access = MB_ACCESS_RW;
    err = mbc_slave_set_descriptor(slave_handle, reg_area);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_slave_set_descriptor COIL failed: %s", esp_err_to_name(err));
        mbc_slave_delete(slave_handle);
        slave_handle = NULL;
        return err;
    }

    // Register Discrete Inputs area
    reg_area.type = MB_PARAM_DISCRETE;
    reg_area.start_offset = 0;  // Start from address 10001
    reg_area.address = (void*)&discrete_reg_params;
    reg_area.size = sizeof(discrete_reg_params_t);
    reg_area.access = MB_ACCESS_RW;
    err = mbc_slave_set_descriptor(slave_handle, reg_area);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_slave_set_descriptor DISCRETE failed: %s", esp_err_to_name(err));
        mbc_slave_delete(slave_handle);
        slave_handle = NULL;
        return err;
    }

    // Start Modbus slave
    err = mbc_slave_start(slave_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "mbc_slave_start failed: %s", esp_err_to_name(err));
        mbc_slave_delete(slave_handle);
        slave_handle = NULL;
        return err;
    }

    ESP_LOGI(TAG, "Modbus TCP Slave started on port %d", MODBUS_TCP_PORT);
    return ESP_OK;
}

/* ==================================================================
 *  REGISTER UPDATE FUNCTIONS
 * ================================================================== */

static void modbus_update_input_registers(void)
{
    static uint32_t uptime_counter = 0;
    uptime_counter++;
    input_reg_params.sys_uptime_sec = (uint16_t)(uptime_counter & 0xFFFF);

    // TODO: Update real values from system
    // input_reg_params.wifi_rssi = get_wifi_rssi();
    // input_reg_params.sta_ip_addr = get_sta_ip();
    // input_reg_params.ap_ip_addr = get_ap_ip();
    // input_reg_params.connected_clients = get_connected_clients();
    // input_reg_params.rtu_tx_count = get_rtu_tx_count();
    // input_reg_params.rtu_rx_count = get_rtu_rx_count();
}

static void modbus_update_discrete_inputs(void)
{
    // TODO: Update real status
    // discrete_reg_params.wifi_sta_connected = wifi_is_sta_connected();
    // discrete_reg_params.wifi_ap_active = wifi_is_ap_active();
    // discrete_reg_params.rtu_link_active = rtu_is_link_active();
}
