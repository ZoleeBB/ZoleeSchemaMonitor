#define _POSIX_C_SOURCE 200809L
#include "app_store.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    SemaphoreHandle_t mutex = malloc(sizeof(*mutex));
    assert(mutex && !pthread_mutex_init(mutex, NULL));
    return mutex;
}
int xSemaphoreTake(SemaphoreHandle_t mutex, unsigned timeout)
{
    (void)timeout;
    return !pthread_mutex_lock(mutex);
}
int xSemaphoreGive(SemaphoreHandle_t mutex)
{
    return !pthread_mutex_unlock(mutex);
}
void esp_fill_random(void *out, size_t length)
{
    FILE *file = fopen("/dev/urandom", "rb");
    assert(file);
    assert(fread(out, 1, length, file) == length);
    fclose(file);
}
esp_err_t bsp_sdcard_mount(void)
{
    return getenv("SD_FAIL") ? ESP_FAIL : ESP_OK;
}
esp_err_t nvs_open(const char *name, int mode, nvs_handle_t *handle)
{
    (void)name;
    (void)mode;
    *handle = 1;
    return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t handle, const char *name, void *data, size_t *size)
{
    (void)handle;
    (void)name;
    FILE *file = fopen("key.bin", "rb");
    if (!file)
        return ESP_ERR_NVS_NOT_FOUND;
    assert(fread(data, 1, *size, file) == *size);
    fclose(file);
    return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t handle, const char *name, const void *data, size_t size)
{
    (void)handle;
    (void)name;
    FILE *file = fopen("key.bin", "wb");
    assert(file && fwrite(data, 1, size, file) == size);
    fclose(file);
    return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}
void nvs_close(nvs_handle_t handle)
{
    (void)handle;
}
