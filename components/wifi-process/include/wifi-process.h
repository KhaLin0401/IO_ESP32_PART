#ifndef WIFI_PROCESS_INCLUDED
#define WIFI_PROCESS_INCLUDED

// Task function
void wifi_process_task(void *pvParameters);

// WiFi control functions
void wifi_process_connect();
void wifi_process_disconnect();
void wifi_process_get_status();

#endif