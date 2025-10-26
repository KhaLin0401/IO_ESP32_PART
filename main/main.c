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

static const char *TAG = "main";

void app_main()
{
    xTaskCreate(
        wifi_process_task,     // Task function
        "wifi_process_task",   // Tên task
        8192,                // Stack size
        NULL,                // Parameters
        5,                   // Priority
        NULL                 // Task handle
    );
    
}