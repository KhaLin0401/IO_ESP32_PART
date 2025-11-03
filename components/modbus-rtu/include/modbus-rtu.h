#pragma once

#ifndef MODBUS_RTU_INCLUDE
#define MODBUS_RTU_INCLUDE

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Start Modbus RTU slave task
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t modbus_rtu_start(void);

/**
 * @brief Stop Modbus RTU slave task
 * @return ESP_OK on success, error code otherwise
 */
esp_err_t modbus_rtu_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* MODBUS_RTU_INCLUDE */
