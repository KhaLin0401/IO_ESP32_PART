#include "modbus-tcp-map.h"

/* ==============================================
 *  GLOBAL VARIABLES DEFINITION
 * ============================================== */

/* Discrete Input Registers (Read-Only) */
discrete_reg_params_t discrete_reg_params = {
    .wifi_sta_connected = 0,
    .wifi_ap_active = 0,
    .rtu_link_active = 0,
    .reserved = 0
};

/* Coil Registers (Read/Write) */
coil_reg_params_t coil_reg_params = {
    .wifi_reconnect_request = 0,
    .ap_enable_request = 0,
    .rtu_bridge_enable = 0,
    .factory_reset_request = 0,
    .reserved = 0
};

/* Input Registers (Read-Only) */
input_reg_params_t input_reg_params = {
    .sys_uptime_sec = 0,
    .wifi_rssi = 0,
    .sta_ip_addr = 0,
    .ap_ip_addr = 0,
    .connected_clients = 0,
    .rtu_tx_count = 0,
    .rtu_rx_count = 0
};

/* Holding Registers (Read/Write) */
holding_reg_params_t holding_reg_params = {
    .wifi_mode = DEFAULT_WIFI_MODE,
    .sta_ssid = {0},
    .sta_pass = {0},
    .ap_ssid = {0},
    .ap_pass = {0},
    .ap_channel = DEFAULT_AP_CHANNEL,
    .ap_max_conn = DEFAULT_MAX_CLIENTS,
    .rtu_slave_addr = MIN_RTU_SLAVE_ADDR,
    .rtu_baudrate = RTU_BAUD_9600,
    .rtu_parity = RTU_PARITY_NONE,
    .tcp_port = DEFAULT_TCP_PORT,
    .save_config_request = 0
};