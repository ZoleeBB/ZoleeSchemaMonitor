#pragma once
#include "esp_err.h"
#include <stdbool.h>
typedef struct
{
    bool connected, recovery, rssi_valid;
    int rssi;
    char ip[16], ssid[33], recovery_ssid[33], recovery_password[17];
} app_network_status_t;
esp_err_t app_network_start(void);
void app_network_status(app_network_status_t *status);
void app_network_restart_later(void);
