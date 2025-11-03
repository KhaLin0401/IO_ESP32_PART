# Kiến Trúc Hệ Thống ESP32 Modbus TCP Gateway

## Tổng Quan

Hệ thống này triển khai một ESP32 Modbus TCP Gateway với khả năng tự động quản lý kết nối WiFi (STA/AP mode) và đồng bộ hóa giữa WiFi và Modbus TCP tasks.

## Sơ Đồ Luồng Hoạt Động

```
┌─────────────┐
│  app_main() │
└──────┬──────┘
       │
       ├─► 1. Khởi tạo NVS Flash
       │
       ├─► 2. Khởi tạo Event Group (đồng bộ WiFi-Modbus)
       │
       ├─► 3. Start WiFi Manager Task
       │
       └─► 4. Start Modbus TCP Task
```

## Các Component Chính

### 1. Main Application (`main/main.c`)

**Chức năng:**
- Điểm khởi đầu của ứng dụng
- Khởi tạo NVS Flash để lưu cấu hình WiFi
- Tạo Event Group để đồng bộ giữa WiFi và Modbus tasks
- Khởi động WiFi Manager Task
- Khởi động Modbus TCP Task

**Events được định nghĩa (`main/app_events.h`):**
```c
#define WIFI_STA_CONNECTED_BIT  BIT0  // WiFi STA kết nối thành công
#define WIFI_AP_STARTED_BIT     BIT1  // WiFi AP Mode đã khởi động
#define WIFI_DISCONNECTED_BIT   BIT2  // WiFi bị ngắt kết nối
```

### 2. WiFi Process Task (`components/wifi-process/`)

**Luồng hoạt động:**
```
┌─────────────────────┐
│ wifi_process_task() │
└──────────┬──────────┘
           │
           ├─► Khởi động WiFi Manager
           │
           ├─► Đăng ký Callbacks:
           │   • WM_EVENT_STA_GOT_IP → cb_connection_ok()
           │   • WM_EVENT_STA_DISCONNECTED → cb_connection_lost()
           │   • WM_ORDER_START_AP → cb_ap_started()
           │
           └─► Loop: Giữ task alive
```

**WiFi Manager Flow:**
```
┌─────────────────┐
│ Đọc Config NVS  │
└────────┬────────┘
         │
    ┌────▼────┐
    │ Có WiFi │
    │  saved? │
    └────┬────┘
         │
    ┌────▼─────────────────────────┐
    │ YES                │ NO      │
    │                    │         │
┌───▼────────┐    ┌──────▼──────┐
│ Kết nối    │    │ Start AP    │
│ STA Mode   │    │ Mode        │
└───┬────┬───┘    └──────┬──────┘
    │    │               │
    │    │               │
┌───▼──┐ │               │
│ Thành│ │               │
│ công │ │               │
└──┬───┘ │               │
   │     │               │
   │  ┌──▼────┐          │
   │  │ Thất  │          │
   │  │ bại?  │          │
   │  └───┬───┘          │
   │      │              │
   │   ┌──▼──────────┐   │
   │   │ Retry <= 3? │   │
   │   └──┬─────┬────┘   │
   │      │YES  │NO      │
   │      │     └────┐   │
   │      │          │   │
   │  ┌───▼───┐  ┌───▼───▼────┐
   │  │ Retry │  │ Start AP    │
   │  └───────┘  │ Mode        │
   │             └────┬────────┘
   │                  │
┌──▼──────────────────▼───────┐
│ Set Event & Notify Modbus   │
│ • WIFI_STA_CONNECTED_BIT    │
│ • WIFI_AP_STARTED_BIT       │
└─────────────────────────────┘
```

**Callbacks:**

1. **`cb_connection_ok()`**: Được gọi khi STA kết nối thành công
   ```c
   → Clear WIFI_DISCONNECTED_BIT
   → Set WIFI_STA_CONNECTED_BIT
   → Log IP address
   ```

2. **`cb_connection_lost()`**: Được gọi khi STA bị ngắt kết nối
   ```c
   → Clear WIFI_STA_CONNECTED_BIT
   → Set WIFI_DISCONNECTED_BIT
   → WiFi Manager tự động retry hoặc start AP
   ```

3. **`cb_ap_started()`**: Được gọi khi AP Mode khởi động
   ```c
   → Set WIFI_AP_STARTED_BIT
   → Modbus có thể khởi động trên AP IP
   ```

### 3. Modbus TCP Task (`components/modbus-tcp/`)

**Luồng hoạt động:**
```
┌──────────────────┐
│ modbus_task()    │
└────────┬─────────┘
         │
         ├─► BƯỚC 1: Chờ WiFi Event
         │   ┌────────────────────────────┐
         │   │ xEventGroupWaitBits()      │
         │   │ • WIFI_STA_CONNECTED_BIT   │
         │   │ • WIFI_AP_STARTED_BIT      │
         │   └───────────┬────────────────┘
         │               │
         ├─► BƯỚC 2: Khởi tạo Modbus TCP
         │   ┌───────────▼────────────────┐
         │   │ modbus_slave_init_tcp()    │
         │   │ • Bind to current IP       │
         │   │ • Start Modbus Slave       │
         │   └───────────┬────────────────┘
         │               │
         ├─► BƯỚC 3: Main Loop
         │   ┌───────────▼─────────────────┐
         │   │ while (running):             │
         │   │  • Check WiFi disconnect     │
         │   │  • Poll Modbus events        │
         │   │  • Update registers          │
         │   └───────────┬─────────────────┘
         │               │
         │          ┌────▼─────┐
         │          │ WiFi     │
         │          │ Ngắt?    │
         │          └────┬─────┘
         │               │
         │               ├─► YES: Cleanup & Restart
         │               │
         │               └─► NO: Continue polling
         │
         └─► BƯỚC 4: Cleanup khi ngắt kết nối
             ┌─────────────────────────────┐
             │ mbc_slave_stop()            │
             │ mbc_slave_delete()          │
             │ → Tự động retry khi WiFi OK │
             └─────────────────────────────┘
```

**Chi tiết xử lý:**

1. **Chờ WiFi Connected:**
   - Task sẽ block cho đến khi nhận được event từ WiFi
   - Chấp nhận cả STA hoặc AP mode
   - Không timeout, chờ mãi mãi

2. **Khởi tạo Modbus:**
   - Bind đến IP hiện tại (STA IP hoặc AP IP)
   - Port mặc định: 502
   - Mode: MB_TCP, IPv4

3. **Polling Loop:**
   - Kiểm tra WiFi disconnect mỗi 10ms
   - Poll Modbus events
   - Cập nhật Input Registers & Discrete Inputs mỗi 1s

4. **Auto Recovery:**
   - Khi phát hiện WiFi disconnect → Stop Modbus
   - Tự động retry khi WiFi reconnect
   - Không cần can thiệp thủ công

## Sequence Diagram - Happy Path (STA Mode)

```
┌──────┐    ┌──────┐    ┌────────────┐    ┌────────┐
│ Main │    │ WiFi │    │ WiFi Mgr   │    │ Modbus │
└───┬──┘    └───┬──┘    └─────┬──────┘    └───┬────┘
    │           │              │                │
    ├─ Init NVS ───────────────┼───────────────►│
    │           │              │                │
    ├─ Init Event Group ───────┼───────────────►│
    │           │              │                │
    ├─ Start WiFi Task ────────►                │
    │           │              │                │
    │           ├─ Start WiFi Manager ─────────►│
    │           │              │                │
    │           │              ├─ Read NVS      │
    │           │              │                │
    │           │              ├─ Connect STA   │
    │           │              │                │
    ├─ Start Modbus Task ──────┼───────────────►│
    │           │              │                │
    │           │              │                ├─ Wait WiFi Event
    │           │              │                │   (blocked)
    │           │              │                │
    │           │         ┌────▼─────┐          │
    │           │         │ STA      │          │
    │           │         │ Connect  │          │
    │           │         │ Success  │          │
    │           │         └────┬─────┘          │
    │           │              │                │
    │           ◄──── Got IP ──┤                │
    │           │              │                │
    │           ├─ cb_connection_ok()           │
    │           │              │                │
    │           ├─ Set WIFI_STA_CONNECTED_BIT ──►
    │           │              │                │
    │           │              │           ┌────▼────┐
    │           │              │           │ Event   │
    │           │              │           │ Received│
    │           │              │           └────┬────┘
    │           │              │                │
    │           │              │      ┌─────────▼────────┐
    │           │              │      │ Init Modbus TCP  │
    │           │              │      │ with current IP  │
    │           │              │      └─────────┬────────┘
    │           │              │                │
    │           │              │      ┌─────────▼────────┐
    │           │              │      │ Start Modbus     │
    │           │              │      │ Slave Server     │
    │           │              │      └─────────┬────────┘
    │           │              │                │
    │           │              │                ├─ Poll Loop
    │           │              │                │  (running)
    └───────────┴──────────────┴────────────────┴─────────
```

## Sequence Diagram - Fallback to AP Mode

```
┌──────┐    ┌──────┐    ┌────────────┐    ┌────────┐
│ Main │    │ WiFi │    │ WiFi Mgr   │    │ Modbus │
└───┬──┘    └───┬──┘    └─────┬──────┘    └───┬────┘
    │           │              │                │
    │           │              ├─ Connect STA   │
    │           │              │   (attempt 1)  │
    │           │              │                │
    │           │         ┌────▼─────┐          │
    │           │         │ Failed   │          │
    │           │         └────┬─────┘          │
    │           │              │                │
    │           │              ├─ Retry (2/3)   │
    │           │              │                │
    │           │         ┌────▼─────┐          │
    │           │         │ Failed   │          │
    │           │         └────┬─────┘          │
    │           │              │                │
    │           │              ├─ Max retry     │
    │           │              │   reached!     │
    │           │              │                │
    │           │         ┌────▼─────┐          │
    │           │         │ Start AP │          │
    │           │         │ Mode     │          │
    │           │         └────┬─────┘          │
    │           │              │                │
    │           ◄─ AP Started ─┤                │
    │           │              │                │
    │           ├─ cb_ap_started()              │
    │           │              │                │
    │           ├─ Set WIFI_AP_STARTED_BIT ────►│
    │           │              │                │
    │           │              │           ┌────▼────┐
    │           │              │           │ Event   │
    │           │              │           │ Received│
    │           │              │           └────┬────┘
    │           │              │                │
    │           │              │      ┌─────────▼────────┐
    │           │              │      │ Init Modbus TCP  │
    │           │              │      │ on AP IP         │
    │           │              │      │ (192.168.4.1)    │
    │           │              │      └─────────┬────────┘
    │           │              │                │
    │           │              │                ├─ Running
    │           │              │                │  on AP IP
    └───────────┴──────────────┴────────────────┴─────────
```

## Xử Lý Disconnect & Recovery

```
┌─────────────────────────────────────────────┐
│ Modbus đang chạy bình thường                │
└───────────────────┬─────────────────────────┘
                    │
         ┌──────────▼──────────┐
         │ WiFi Disconnected!  │
         └──────────┬──────────┘
                    │
         ┌──────────▼──────────────────┐
         │ WiFi Task:                  │
         │ • cb_connection_lost()      │
         │ • Set WIFI_DISCONNECTED_BIT │
         └──────────┬──────────────────┘
                    │
         ┌──────────▼──────────────────┐
         │ Modbus Task:                │
         │ • Detect disconnect bit     │
         │ • Break from poll loop      │
         │ • Stop & Delete slave       │
         └──────────┬──────────────────┘
                    │
         ┌──────────▼──────────────────┐
         │ WiFi Manager:               │
         │ • Auto retry STA (3 times)  │
         │ • Or start AP mode          │
         └──────────┬──────────────────┘
                    │
         ┌──────────▼──────────────────┐
         │ Reconnected!                │
         │ • Set WIFI_STA_CONNECTED_BIT│
         │   or WIFI_AP_STARTED_BIT    │
         └──────────┬──────────────────┘
                    │
         ┌──────────▼──────────────────┐
         │ Modbus Task:                │
         │ • Auto restart init         │
         │ • Re-create slave           │
         │ • Resume polling            │
         └─────────────────────────────┘
```

## Files Structure

```
Module_IO/
├── main/
│   ├── main.c              # App entry point
│   ├── app_events.h        # Event definitions
│   └── CMakeLists.txt
│
├── components/
│   ├── wifi-process/
│   │   ├── wifi-process.c      # WiFi task & callbacks
│   │   ├── include/
│   │   │   └── wifi-process.h
│   │   └── CMakeLists.txt
│   │
│   ├── modbus-tcp/
│   │   ├── modbus-tcp.c        # Modbus task
│   │   ├── modbus-tcp-map.c    # Register mapping
│   │   ├── include/
│   │   │   ├── modbus-tcp.h
│   │   │   └── modbus-tcp-map.h
│   │   └── CMakeLists.txt
│   │
│   └── esp32-wifi-manager/    # Third-party WiFi Manager
│       └── ...
│
└── docs/
    └── system-architecture.md  # This file
```

## Configuration

### WiFi Settings (via NVS)
- SSID & Password được lưu trong NVS Flash
- Có thể cấu hình qua Web Portal khi ở AP Mode
- URL: http://192.168.4.1

### Modbus Settings
- Port: 502 (default)
- Slave ID: 1
- Mode: TCP
- Registers: Xem `modbus-tcp-map.h`

## Build & Flash

```bash
# Build
idf.py build

# Flash
idf.py -p COMX flash monitor

# Hoặc
idf.py flash monitor
```

## Logs Example

```
I (xxx) APP_MAIN: ╔════════════════════════════════════════╗
I (xxx) APP_MAIN: ║   ESP32 Modbus TCP Gateway Starting    ║
I (xxx) APP_MAIN: ╚════════════════════════════════════════╝
I (xxx) APP_MAIN: Khởi tạo NVS Flash...
I (xxx) APP_MAIN: ✓ NVS Flash khởi tạo thành công
I (xxx) APP_MAIN: Khởi tạo Event Group...
I (xxx) APP_MAIN: ✓ Event Group khởi tạo thành công
I (xxx) APP_MAIN: Khởi động WiFi Manager Task...
I (xxx) APP_MAIN: ✓ WiFi Task đã khởi động
I (xxx) APP_MAIN: Khởi động Modbus TCP Task...
I (xxx) APP_MAIN: ✓ Modbus TCP Task đã khởi động
I (xxx) APP_MAIN: ╔════════════════════════════════════════╗
I (xxx) APP_MAIN: ║   Hệ thống đã khởi động hoàn tất      ║
I (xxx) APP_MAIN: ╚════════════════════════════════════════╝

I (xxx) WIFI_PROCESS: Đang khởi động WiFi Manager...
I (xxx) WIFI_PROCESS: ✓ WiFi Manager đã khởi động
I (xxx) WIFI_PROCESS: → Đang đọc config từ NVS và thử kết nối...

I (xxx) MODBUS_TCP: Modbus TCP task đã khởi động
I (xxx) MODBUS_TCP: → Đang chờ WiFi kết nối (STA hoặc AP)...

I (xxx) WIFI_PROCESS: ✓ WiFi STA kết nối thành công! IP: 192.168.1.100
I (xxx) WIFI_PROCESS: → Event WIFI_STA_CONNECTED_BIT đã được set

I (xxx) MODBUS_TCP: ✓ WiFi STA đã kết nối, đang khởi động Modbus TCP...
I (xxx) MODBUS_TCP: ✓ Modbus TCP Slave đang chạy
```

## Troubleshooting

### 1. Modbus không khởi động
**Nguyên nhân:** WiFi chưa kết nối
**Giải pháp:** Kiểm tra logs của WiFi Process, đảm bảo event bits được set

### 2. WiFi bị ngắt liên tục
**Nguyên nhân:** Signal yếu hoặc router issue
**Giải pháp:** WiFi Manager sẽ tự động retry 3 lần, sau đó chuyển sang AP Mode

### 3. Modbus timeout
**Nguyên nhân:** WiFi disconnect trong lúc xử lý request
**Giải pháp:** Modbus sẽ tự động stop và restart khi WiFi reconnect

## Future Improvements

1. **Thêm MQTT** để report status
2. **OTA Update** qua WiFi
3. **Web Dashboard** để monitor Modbus traffic
4. **Modbus RTU Gateway** (ESP32 as bridge RTU ↔ TCP)
5. **TLS/SSL** cho Modbus TCP security

## References

- [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)
- [ESP32 WiFi Manager](https://github.com/tonyp7/esp32-wifi-manager)
- [ESP-Modbus Library](https://github.com/espressif/esp-modbus)
- [Modbus Protocol Specification](https://modbus.org/docs/Modbus_Application_Protocol_V1_1b3.pdf)

