# Modbus TCP Register Map

## WiFi Configuration Registers (Read/Write)

| Address | Name | Description | Data Type | Valid Values |
|---------|------|-------------|------------|--------------|
| 1000 | WiFi Mode | Operating mode of WiFi | UINT16 | 0: Station (STA)<br>1: Access Point (AP)<br>2: Station + AP |
| 1001 | Connection Status | Current WiFi connection status (Read Only) | UINT16 | 0: Disconnected<br>1: Connected<br>2: Connecting |
| 1002 | Signal Strength | WiFi signal strength in dBm (Read Only) | INT16 | -100 to 0 |

## Station Mode Configuration (Read/Write)

| Address | Name | Description | Data Type | Valid Values |
|---------|------|-------------|------------|--------------|
| 1100-1109 | STA SSID | Station mode SSID (20 chars max) | STRING | ASCII characters |
| 1110-1119 | STA Password | Station mode password (20 chars max) | STRING | ASCII characters |
| 1120 | STA Power Save | Power save mode for station | UINT16 | 0: Disabled<br>1: Enabled |
| 1121 | STA IP Config | IP address configuration method | UINT16 | 0: DHCP<br>1: Static IP |

## Access Point Configuration (Read/Write)

| Address | Name | Description | Data Type | Valid Values |
|---------|------|-------------|------------|--------------|
| 1200-1209 | AP SSID | Access point SSID (20 chars max) | STRING | ASCII characters |
| 1210-1219 | AP Password | Access point password (20 chars max) | STRING | ASCII characters |
| 1220 | AP Channel | WiFi channel for AP mode | UINT16 | 1-13 |
| 1221 | AP Hidden | Hide SSID broadcast | UINT16 | 0: Visible<br>1: Hidden |
| 1222 | AP Bandwidth | Channel bandwidth | UINT16 | 0: 20MHz<br>1: 40MHz |

## Static IP Configuration (Read/Write when STA IP Config = 1)

| Address | Name | Description | Data Type | Valid Values |
|---------|------|-------------|------------|--------------|
| 1300-1301 | Static IP | Static IP address | UINT32 | Valid IPv4 address |
| 1302-1303 | Subnet Mask | Network subnet mask | UINT32 | Valid subnet mask |
| 1304-1305 | Gateway | Default gateway address | UINT32 | Valid IPv4 address |

## Control Registers (Write Only)

| Address | Name | Description | Data Type | Valid Values |
|---------|------|-------------|------------|--------------|
| 2000 | WiFi Control | Control commands | UINT16 | 1: Connect<br>2: Disconnect<br>3: Start Scan<br>4: Save Config |
| 2001 | Reset | Reset device | UINT16 | 1: Soft Reset<br>2: Factory Reset |

## Status Registers (Read Only)

| Address | Name | Description | Data Type | Notes |
|---------|------|-------------|------------|--------|
| 3000 | Error Code | Last error code | UINT16 | 0: No Error<br>1: Connection Failed<br>2: Auth Failed<br>3: Network Not Found |
| 3001 | Scan Status | WiFi scan status | UINT16 | 0: Idle<br>1: Scanning<br>2: Scan Complete |
| 3002 | AP Count | Number of APs found in scan | UINT16 | 0-255 |

Notes:
- All string registers store 2 characters per register (16-bit)
- Write operations to read-only registers will be ignored
- Invalid values will return error code via register 3000
- Changes to configuration take effect after sending control command to save config (2000:4)
