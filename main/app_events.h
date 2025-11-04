#ifndef APP_EVENTS_H
#define APP_EVENTS_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

#ifdef __cplusplus
extern "C" {
#endif

// Event bits cho WiFi và Modbus
#define WIFI_STA_CONNECTED_BIT  BIT0
#define WIFI_AP_STARTED_BIT     BIT1
#define WIFI_DISCONNECTED_BIT   BIT2

// Global Event Group handle
extern EventGroupHandle_t app_event_group;

#ifdef __cplusplus
}
#endif

#endif /* APP_EVENTS_H */

