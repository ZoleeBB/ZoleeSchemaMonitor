#pragma once
#include "esp_err.h"
#include "schema_model.h"

/* Minden visszaadott JSON a hívóé. IO közben nincs kijelzőzár. */
esp_err_t app_store_init(void);
bool app_store_ready(void);
bool app_store_recovered(void);
uint32_t app_store_revision(void);
cJSON *app_store_project(uint32_t *revision);
cJSON *app_store_settings(bool public_view);
esp_err_t app_store_save_project(const cJSON *project,
                                 uint32_t expected,
                                 uint32_t *revision,
                                 char error[SM_ERROR_SIZE]);
esp_err_t app_store_save_settings(const cJSON *patch, bool loxone, char error[SM_ERROR_SIZE]);

/* A teljes beállításokat tartalmazza: csak titkosításhoz, az eredmény nem publikus. */
cJSON *app_store_snapshot(void);
esp_err_t app_store_restore(const cJSON *state,
                            uint32_t expected,
                            uint32_t *revision,
                            char error[SM_ERROR_SIZE]);
