#ifndef WIFI_PROCESS_INCLUDED
#define WIFI_PROCESS_INCLUDED
#include "stdbool.h"
#include "esp_err.h"
#include "esp_netif.h"
typedef struct {
    bool use_static_ip;
    esp_netif_ip_info_t ip_info;
    esp_ip4_addr_t dns_main;
    esp_ip4_addr_t dns_backup;
} wifi_static_ip_config_t;

extern wifi_static_ip_config_t wifi_static_ip_config;

// Task function
void wifi_process_task(void *pvParameters);

// Initialization functions
esp_err_t init_nvs(void);
esp_err_t init_event_group(void);
esp_err_t start_wifi_task(void);

// Internal functions - not exported
// static esp_err_t wifi_init(void);
// static void wifi_process_config_static_ip(void);


#endif