#pragma once
#include "schema_loxone.h"
#include "esp_err.h"
esp_err_t app_loxone_start(void);
bool app_loxone_read(const char *uuid, double *value, void *context);
bool app_loxone_connected(void);
cJSON *app_loxone_catalog(void);
cJSON *app_loxone_live(void);
