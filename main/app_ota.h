#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
/* A reader legfeljebb capacity bájtot adhat vissza; 0/-1 megszakítás. */
typedef int (*app_ota_read_fn)(void *context, void *buffer, size_t capacity);
esp_err_t app_ota_upload(size_t size, app_ota_read_fn reader, void *context);
/* A külső kiadás SHA/azonosító ellenőrzése is a bootcél módosítása ELŐTT fut. */
esp_err_t app_ota_upload_verified(size_t size,
                                  app_ota_read_fn reader,
                                  void *context,
                                  bool (*verify)(void *context));
/* Sikeres alapindítás + 30 s után véglegesítés; 180 s hibás indítás után rollback. */
void app_ota_boot_poll(bool healthy, uint64_t uptime_seconds);
