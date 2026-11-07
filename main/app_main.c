/* P4 indítás, tartós SD-állapot, helyi webszerkesztő és diagnosztika. */
#include <stdio.h>
#include <string.h>

#include "app_diagnostics.h"
#include "app_diagnostic_view.h"
#include "app_events.h"
#include "app_network.h"
#include "app_store.h"
#include "app_web.h"
#include "app_ota.h"
#include "app_scene.h"
#include "app_loxone.h"
#include "bsp/esp-bsp.h"
#include "esp_app_desc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "schema_core.h"

/* A tájolás induláskor rögzül. A natív panel 800×1280-as, 90/270° fekvő.
 * A touch-tengelyek minden állásban ugyanazt az elfordítást követik. */
static esp_lv_adapter_rotation_t rotation_value(unsigned degrees)
{
    switch (degrees)
    {
        case 90:
            return ESP_LV_ADAPTER_ROTATE_90;
        case 180:
            return ESP_LV_ADAPTER_ROTATE_180;
        case 270:
            return ESP_LV_ADAPTER_ROTATE_270;
        default:
            return ESP_LV_ADAPTER_ROTATE_0;
    }
}

static unsigned read_rotation(void)
{
    cJSON *settings = app_store_settings(true);
    unsigned degrees =
        sm_json_number(cJSON_GetObjectItemCaseSensitive(settings, "display"), "rotation", 270);
    cJSON_Delete(settings);
    return sm_rotation_valid(degrees) ? degrees : 270;
}

void app_main(void)
{
    esp_err_t error = nvs_flash_init();
    if (error == ESP_ERR_NVS_NO_FREE_PAGES || error == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        error = nvs_flash_init();
    }
    ESP_ERROR_CHECK(error);
    app_events_add(APP_EVENT_BOOT);
    error = app_store_init();
    app_events_add(error == ESP_OK ? APP_EVENT_SD_READY : APP_EVENT_SD_MISSING);
    if (app_store_recovered())
        app_events_add(APP_EVENT_SD_RECOVERED);
    if (error != ESP_OK)
        ESP_LOGW("schema", "SD-allapot nem elerheto: %s", esp_err_to_name(error));
    unsigned degrees = read_rotation();
    bool swap = degrees == 90 || degrees == 270;
    bsp_display_cfg_t cfg = {.lv_adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG(),
                             .rotation = rotation_value(degrees),
                             .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
                             .touch_flags = {.swap_xy = swap,
                                             .mirror_x = degrees == 90 || degrees == 180,
                                             .mirror_y = degrees == 180 || degrees == 270}};
    cfg.lv_adapter_cfg.task_stack_size = 32768;
    cfg.lv_adapter_cfg.stack_in_psram = true;
    cfg.lv_adapter_cfg.task_core_id = 1;
    lv_display_t *display = bsp_display_start_with_config(&cfg);
    if (!display)
    {
        ESP_LOGE("schema", "A kijelzo nem indithato");
        app_ota_boot_poll(false, 180);
        return;
    }
    bsp_display_backlight_on();
    app_diagnostic_view_poll();
    bool web_ready = false;
    error = app_network_start();
    if (error != ESP_OK)
        ESP_LOGE("schema", "Wi-Fi: %s", esp_err_to_name(error));
    else
    {
        error = app_loxone_start();
        if (error != ESP_OK)
            ESP_LOGE("schema", "Loxone: %s", esp_err_to_name(error));
        app_scene_set_reader(app_loxone_read, NULL);
        error = app_web_start();
        web_ready = error == ESP_OK;
        if (error != ESP_OK)
            ESP_LOGE("schema", "HTTP: %s", esp_err_to_name(error));
    }
    cJSON *appearance = app_store_settings(true);
    app_scene_configure(appearance);
    cJSON_Delete(appearance);
    /* A meres a kijelzozaron kivul fut, azonos szoveg nem ujrarajzolas. */
    bool diagnostics_ready = app_diagnostics_start() == ESP_OK;
    if (!diagnostics_ready)
        ESP_LOGE("schema", "A diagnosztikai task nem indithato");
    int64_t previous_tick = esp_timer_get_time();
    unsigned metrics_ticks = 0;
    while (true)
    {
        vTaskDelay(pdMS_TO_TICKS(50));
        app_scene_poll();
        app_diagnostic_view_poll();
        int64_t now = esp_timer_get_time();
        app_scene_update((unsigned)((now - previous_tick) / 1000));
        previous_tick = now;
        if (++metrics_ticks % 20)
            continue;
        app_diagnostics_t metrics;
        app_diagnostics_capture(&metrics);
        app_ota_boot_poll(web_ready && diagnostics_ready && metrics.sampled &&
                              metrics.sample_age <= 10 && app_store_ready(),
                          now / 1000000);
        app_network_status_t network;
        app_network_status(&network);
        app_diagnostic_view_update(&metrics, &network);
    }
}
