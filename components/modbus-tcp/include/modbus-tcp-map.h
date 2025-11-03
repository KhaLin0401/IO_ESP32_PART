#pragma once
#ifndef MODBUS_TCP_MAP_INCLUDE
#define MODBUS_TCP_MAP_INCLUDE

#include <stdint.h>
#include "sdkconfig.h"

/* ==============================================
 *  SYSTEM CONSTANTS & VALIDATION LIMITS
 * ============================================== */
#define MAX_SSID_LENGTH          32
#define MAX_PASSWORD_LENGTH      32
#define MIN_TCP_PORT             1024
#define MAX_TCP_PORT             65535
#define MIN_AP_CHANNEL           1
#define MAX_AP_CHANNEL           13
#define MAX_AP_CLIENTS           4
#define MIN_RTU_SLAVE_ADDR       1
#define MAX_RTU_SLAVE_ADDR       247

#define MODBUS_TCP_PORT          502

/* ==============================================
 *  REGISTER ADDRESS MAP
 * ============================================== */
/* WiFi Configuration Registers */
#define REG_WIFI_MODE                1000
#define REG_WIFI_CONN_STATUS         1001
#define REG_WIFI_SIGNAL_STRENGTH     1002

/* Station Mode Configuration */
#define REG_STA_SSID_START           1100
#define REG_STA_SSID_END             1109
#define REG_STA_PWD_START            1110
#define REG_STA_PWD_END              1119
#define REG_STA_POWER_SAVE           1120
#define REG_STA_IP_CONFIG            1121

/* Access Point Configuration */
#define REG_AP_SSID_START            1200
#define REG_AP_SSID_END              1209
#define REG_AP_PWD_START             1210
#define REG_AP_PWD_END               1219
#define REG_AP_CHANNEL               1220
#define REG_AP_HIDDEN                1221
#define REG_AP_BANDWIDTH             1222

/* Static IP Configuration */
#define REG_STATIC_IP_START          1300
#define REG_STATIC_IP_END            1301
#define REG_SUBNET_MASK_START        1302
#define REG_SUBNET_MASK_END          1303
#define REG_GATEWAY_START            1304
#define REG_GATEWAY_END              1305

/* Control Registers */
#define REG_WIFI_CONTROL             2000
#define REG_RESET                    2001

/* Status Registers */
#define REG_ERROR_CODE               3000
#define REG_SCAN_STATUS              3001
#define REG_AP_COUNT                 3002

/* ==============================================
 *  DEFAULTS
 * ============================================== */
#define DEFAULT_WIFI_MODE            WIFI_MODE_STA
#define DEFAULT_STA_POWER_SAVE       0
#define DEFAULT_STA_IP_CONFIG        0
#define DEFAULT_AP_CHANNEL           1
#define DEFAULT_AP_HIDDEN            0
#define DEFAULT_AP_BANDWIDTH         0
#define DEFAULT_TCP_PORT             MODBUS_TCP_PORT
#define DEFAULT_MAX_CLIENTS          MAX_AP_CLIENTS

/* ==============================================
 *  ENUMERATIONS
 * ============================================== */
#define WIFI_MODE_STA                0
#define WIFI_MODE_AP                 1
#define WIFI_MODE_APSTA              2

#define RTU_BAUD_9600                1
#define RTU_BAUD_19200               2
#define RTU_BAUD_38400               3
#define RTU_BAUD_57600               4
#define RTU_BAUD_115200              5

#define RTU_PARITY_NONE              0
#define RTU_PARITY_ODD               1
#define RTU_PARITY_EVEN              2

#define WIFI_STATUS_DISCONNECTED     0
#define WIFI_STATUS_CONNECTED        1
#define WIFI_STATUS_CONNECTING       2

#define WIFI_CTRL_CONNECT            1
#define WIFI_CTRL_DISCONNECT         2
#define WIFI_CTRL_START_SCAN         3
#define WIFI_CTRL_SAVE_CONFIG        4

#define RESET_SOFT                   1
#define RESET_FACTORY                2

#define ERROR_NONE                   0
#define ERROR_CONN_FAILED            1
#define ERROR_AUTH_FAILED            2

#pragma pack(push, 1)

/* ==============================================
 *  MODBUS REGISTER STRUCTURES
 * ============================================== */
typedef struct {
    uint8_t wifi_sta_connected     :1;
    uint8_t wifi_ap_active         :1;
    uint8_t rtu_link_active        :1;
    uint8_t reserved               :5;
} discrete_reg_params_t;

typedef struct {
    uint8_t wifi_reconnect_request :1;
    uint8_t ap_enable_request      :1;
    uint8_t rtu_bridge_enable      :1;
    uint8_t factory_reset_request  :1;
    uint8_t reserved               :4;
} coil_reg_params_t;

typedef struct {
    uint16_t sys_uptime_sec;      // 30001
    int16_t  wifi_rssi;           // 30002
    uint32_t sta_ip_addr;         // 30003–30004
    uint32_t ap_ip_addr;          // 30005–30006
    uint16_t connected_clients;   // 30007
    uint16_t rtu_tx_count;        // 30008
    uint16_t rtu_rx_count;        // 30009
} input_reg_params_t;

typedef struct {
    uint16_t wifi_mode;                 // 40001
    uint16_t sta_ssid[MAX_SSID_LENGTH / 2]; // 40002–40017 (UTF-16 modbus mapping)
    uint16_t sta_pass[MAX_PASSWORD_LENGTH / 2]; // 40018–40033
    uint16_t ap_ssid[MAX_SSID_LENGTH / 2];  // 40034–40049
    uint16_t ap_pass[MAX_PASSWORD_LENGTH / 2]; // 40050–40065
    uint16_t ap_channel;                // 40066
    uint16_t ap_max_conn;               // 40067
    uint16_t rtu_slave_addr;            // 40068
    uint16_t rtu_baudrate;              // 40069
    uint16_t rtu_parity;                // 40070
    uint16_t tcp_port;                  // 40071
    uint16_t save_config_request;       // 40072
} holding_reg_params_t;

#pragma pack(pop)

/* ==============================================
 *  EXTERNAL VARIABLES
 * ============================================== */
extern holding_reg_params_t holding_reg_params;
extern input_reg_params_t input_reg_params;
extern coil_reg_params_t coil_reg_params;
extern discrete_reg_params_t discrete_reg_params;

#endif /* MODBUS_TCP_MAP_INCLUDE */