/* Idle-időből számolt CPU, külön belső RAM/PSRAM/DMA és ritkított SD-mérés.
 * Forrásminta: ZoleeEnergyFlowDisplay 2dabb18, app_diagnostics.c. */
#include "app_diagnostics.h"
#include "app_network.h"
#include "app_loxone.h"
#include "app_release.h"
#include "app_store.h"
#include "esp_app_desc.h"
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static atomic_flag s_sampling = ATOMIC_FLAG_INIT;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static app_diagnostics_t s_snapshot;
static uint64_t s_idle[2];
static int64_t s_cpu_time, s_next_sd;
static TaskHandle_t s_main_task;
static bool s_started;
static bool s_cpu_started, s_memory_warned, s_stack_warned;

/* Csak tényleges rajzolást követő befejezés érkezik, részpufferenként nincs esemény. */
static portMUX_TYPE s_frame_lock = portMUX_INITIALIZER_UNLOCKED;
static unsigned s_frames, s_max_render_us;
static bool s_frame_seen;
static int64_t s_frame_window = -1;

void app_diagnostics_frame_completed(unsigned render_us)
{
    portENTER_CRITICAL(&s_frame_lock);
    s_frame_seen = true;
    s_frames++;
    if (render_us > s_max_render_us)
        s_max_render_us = render_us;
    portEXIT_CRITICAL(&s_frame_lock);
}

static void sample_frames(app_diagnostics_t *next, int64_t now)
{
    portENTER_CRITICAL(&s_frame_lock);
    unsigned frames = s_frames, maximum = s_max_render_us;
    bool seen = s_frame_seen;
    s_frames = 0;
    s_max_render_us = 0;
    portEXIT_CRITICAL(&s_frame_lock);
    next->display_valid = seen && s_frame_window >= 0 && now > s_frame_window;
    next->display_frames = frames;
    next->max_render_us = maximum;
    next->display_fps = next->display_valid ? frames * 1000000.0 / (now - s_frame_window) : 0;
    s_frame_window = now;
}

void app_diagnostics_capture(app_diagnostics_t *out)
{
    portENTER_CRITICAL(&s_lock);
    *out = s_snapshot;
    portEXIT_CRITICAL(&s_lock);
    int64_t age = esp_timer_get_time() / 1000000 - (int64_t)out->uptime_seconds;
    out->sample_age = out->sampled && age > 0 ? (uint64_t)age : 0;
}
void app_diagnostics_poll(void)
{
    if (atomic_flag_test_and_set(&s_sampling))
        return;
    app_diagnostics_t next;
    app_diagnostics_capture(&next);
    int64_t now = esp_timer_get_time();
    sample_frames(&next, now);
    if (!s_cpu_started || now - s_cpu_time >= 2000000)
    {
        for (unsigned core = 0; core < 2; core++)
        {
            TaskStatus_t task;
            vTaskGetInfo(xTaskGetIdleTaskHandleForCore(core), &task, pdFALSE, eInvalid);
            uint64_t idle = task.ulRunTimeCounter;
            if (s_cpu_started)
            {
                uint64_t elapsed = now - s_cpu_time, delta = idle - s_idle[core];
                next.cpu[core] = delta >= elapsed ? 0 : 100 - (unsigned)(delta * 100 / elapsed);
            }
            s_idle[core] = idle;
        }
        next.cpu_valid = s_cpu_started;
        s_cpu_started = true;
        s_cpu_time = now;
    }
    next.sampled = true;
    next.uptime_seconds = now / 1000000;
    next.clock_valid = time(NULL) >= 1704067200;
    next.internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    next.internal_minimum = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    next.internal_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    next.psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    next.psram_total = heap_caps_get_total_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    next.dma_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    next.dma_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
    /* Az ESP-IDF high-water értéke bájt, nem StackType_t elemszám. */
    next.main_stack_free = uxTaskGetStackHighWaterMark(s_main_task);
    next.task_count = uxTaskGetNumberOfTasks();
    bool was_mounted = next.sd_mounted;
    next.sd_mounted = app_store_ready();
    next.storage_recovery = app_store_recovered();
    if (!next.sd_mounted)
        next.sd_space_valid = false;
    else if (!s_next_sd || now >= s_next_sd || !was_mounted)
    {
        bool previous = next.sd_space_valid;
        next.sd_space_valid =
            esp_vfs_fat_info("/sdcard", &next.sd_total, &next.sd_free) == ESP_OK &&
            next.sd_total > 0 && next.sd_free <= next.sd_total;
        if (!next.sd_space_valid && (previous || !s_next_sd))
            app_events_add(APP_EVENT_SD_SPACE);
        s_next_sd = now + 10000000;
    }
    app_network_status_t network;
    app_network_status(&network);
    next.wifi_connected = network.connected;
    next.recovery_wifi = network.recovery;
    next.rssi = network.rssi;
    next.rssi_valid = network.rssi_valid;
    snprintf(next.ip,
             sizeof(next.ip),
             "%s",
             network.connected  ? network.ip
             : network.recovery ? "192.168.4.1"
                                : "");
    snprintf(next.ssid, sizeof(next.ssid), "%s", network.ssid);
    snprintf(next.recovery_ssid, sizeof(next.recovery_ssid), "%s", network.recovery_ssid);
    snprintf(next.firmware, sizeof(next.firmware), "%.32s", esp_app_get_description()->version);
    app_loxone_diagnostics_t loxone;
    app_loxone_diagnostics(&loxone);
    next.loxone_connected = loxone.connected;
    next.numeric_age = loxone.numeric_age;
    next.known_values = loxone.known_values;
    cJSON *update = app_release_status();
    snprintf(next.update_phase, sizeof(next.update_phase), "%s", sm_json_string(update, "phase"));
    snprintf(
        next.update_version, sizeof(next.update_version), "%s", sm_json_string(update, "version"));
    next.update_received = sm_json_number(update, "receivedBytes", 0);
    next.update_busy = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(update, "busy"));
    next.update_success = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(update, "success"));
    cJSON_Delete(update);
    if (!s_memory_warned && next.internal_total && next.internal_minimum < 8192)
    {
        app_events_add(APP_EVENT_MEMORY_LOW);
        s_memory_warned = true;
    }
    if (!s_stack_warned && next.main_stack_free < 1024)
    {
        app_events_add(APP_EVENT_STACK_LOW);
        s_stack_warned = true;
    }
    next.event_count = app_events_snapshot(next.events);
    portENTER_CRITICAL(&s_lock);
    s_snapshot = next;
    portEXIT_CRITICAL(&s_lock);
    atomic_flag_clear(&s_sampling);
}

static void sampler(void *argument)
{
    (void)argument;
    while (true)
    {
        app_diagnostics_poll();
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
esp_err_t app_diagnostics_start(void)
{
    if (s_started)
        return ESP_ERR_INVALID_STATE;
    s_main_task = xTaskGetCurrentTaskHandle();
    if (xTaskCreateWithCaps(
            sampler, "schema_diag", 8192, NULL, 1, NULL, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) !=
        pdPASS)
        return ESP_ERR_NO_MEM;
    s_started = true;
    return ESP_OK;
}
