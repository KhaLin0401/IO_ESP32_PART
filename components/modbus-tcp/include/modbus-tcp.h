#pragma once

#ifndef MODBUS_TCP_INCLUDE
#define MODBUS_TCP_INCLUDE

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start Modbus TCP slave task
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t modbus_tcp_start(void);

/**
 * @brief Stop Modbus TCP slave task
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t modbus_tcp_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_TCP_INCLUDE */