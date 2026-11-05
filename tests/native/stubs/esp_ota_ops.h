#pragma once
#include "esp_err.h"
#include <stddef.h>
typedef struct
{
    size_t size;
} esp_partition_t;
typedef unsigned esp_ota_handle_t;
typedef enum
{
    ESP_OTA_IMG_VALID,
    ESP_OTA_IMG_PENDING_VERIFY
} esp_ota_img_states_t;
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start);
const esp_partition_t *esp_ota_get_running_partition(void);
esp_err_t esp_ota_get_state_partition(const esp_partition_t *part, esp_ota_img_states_t *state);
esp_err_t esp_ota_begin(const esp_partition_t *part, size_t size, esp_ota_handle_t *handle);
esp_err_t esp_ota_write(esp_ota_handle_t handle, const void *data, size_t size);
esp_err_t esp_ota_end(esp_ota_handle_t handle);
esp_err_t esp_ota_abort(esp_ota_handle_t handle);
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *part);
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void);
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void);
