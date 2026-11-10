#pragma once
#include "app_events.h"
#include "schema_model.h"
#include "esp_err.h"

typedef struct
{
    bool sampled, cpu_valid, sd_space_valid, sd_mounted, storage_recovery, clock_valid;
    uint64_t uptime_seconds, sample_age, sd_total, sd_free;
    bool display_valid;
    double display_fps;
    int64_t display_window_end_us, display_window_us;
    unsigned display_frames, over_50ms_frames, max_render_us;
    unsigned cpu[2], internal_free, internal_minimum, internal_total;
    unsigned psram_free, psram_total, dma_free, dma_largest, main_stack_free, task_count;
    bool wifi_connected, recovery_wifi, loxone_connected, rssi_valid;
    int rssi;
    char ip[16], ssid[33], recovery_ssid[33], firmware[33];
    int64_t numeric_age;
    unsigned known_values;
    char update_phase[32], update_version[65];
    unsigned update_received;
    bool update_busy, update_success;
    app_event_t events[APP_EVENT_LIMIT];
    size_t event_count;
} app_diagnostics_t;

/* Az app_main egyszer indítja. A mintavétel külön PSRAM-stackű taskból fut;
 * az SD-kapacitást legfeljebb 10 s-onként, kijelzőzár nélkül olvassa. */
esp_err_t app_diagnostics_start(void);
void app_diagnostics_frame_completed(unsigned render_us);
/* Kizárólag a mintavételi task és a gazdagépes teszt hívja. */
void app_diagnostics_poll(void);
/* Rövid, csak RAM-ot olvasó pillanatkép. A CPU első mérése ismeretlen. */
void app_diagnostics_capture(app_diagnostics_t *out);
cJSON *app_diagnostics_json(void);
const char *app_diagnostics_phase_text(const char *phase, bool english);
