#pragma once
#include "schema_model.h"
#include "esp_err.h"
esp_err_t app_release_init(void);
esp_err_t app_release_start(bool firmware);
cJSON *app_release_status(void);
/* Csak a projekt kiadási tárának HTTPS-végpontjai használhatók. */
bool app_release_url_allowed(const char *url);
bool app_release_descriptor_valid(const cJSON *descriptor);
