#pragma once
#include "esp_err.h"
esp_err_t bsp_sdcard_mount(void);
#ifdef SCHEMA_SCENE_TEST
#include <stdbool.h>
bool bsp_display_lock(unsigned timeout);
void bsp_display_unlock(void);
#endif
