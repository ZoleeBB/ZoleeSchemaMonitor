/* Közös HU/EN eseményszövegek és titokmentes webes pillanatkép. */
#include "app_diagnostics.h"
#include <string.h>

typedef struct
{
    const char *code, *hu, *en;
    unsigned level;
} event_text_t;
static const event_text_t s_texts[APP_EVENT_COUNT] = {
    {"boot", "A készülék elindult", "Device started", 0},
    {"wifi.up", "Wi-Fi kapcsolat létrejött", "Wi-Fi connected", 0},
    {"wifi.down", "Wi-Fi kapcsolat megszakadt", "Wi-Fi disconnected", 1},
    {"wifi.recovery", "Beállító Wi-Fi elindult", "Setup Wi-Fi started", 1},
    {"sd.ready", "SD-állapottár elérhető", "SD state storage ready", 0},
    {"sd.missing", "SD-állapottár nem érhető el", "SD state storage unavailable", 2},
    {"sd.recovered", "Korábbi érvényes mentés visszaállítva", "Earlier valid state recovered", 1},
    {"sd.write", "Az SD-mentés nem sikerült", "SD save failed", 2},
    {"sd.space", "Az SD-kapacitás nem olvasható", "SD capacity could not be read", 1},
    {"loxone.up", "Loxone kapcsolat létrejött", "Loxone connected", 0},
    {"loxone.down", "Loxone kapcsolat megszakadt", "Loxone disconnected", 1},
    {"update.start", "Frissítés indult", "Update started", 0},
    {"update.done", "Frissítés ellenőrizve", "Update verified", 0},
    {"update.failed", "A frissítés nem sikerült", "Update failed", 2},
    {"memory.low", "Kevés belső RAM: alacsony mélypont", "Internal RAM low-water warning", 1},
    {"stack.low", "Kevés az app_main szabad verme", "app_main stack low-water warning", 1}};
const char *app_event_code(app_event_code_t code)
{
    return code >= 0 && code < APP_EVENT_COUNT ? s_texts[code].code : "unknown";
}
const char *app_event_text(app_event_code_t code, bool english)
{
    return code >= 0 && code < APP_EVENT_COUNT ? (english ? s_texts[code].en : s_texts[code].hu)
                                               : "?";
}
unsigned app_event_level(app_event_code_t code)
{
    return code >= 0 && code < APP_EVENT_COUNT ? s_texts[code].level : 1;
}
const char *app_diagnostics_phase_text(const char *phase, bool english)
{
    static const char *texts[][3] = {
        {"idle", "Nincs frissítés", "Idle"},
        {"release", "Kiadás keresése", "Checking release"},
        {"clock", "Időszinkronra vár", "Waiting for clock"},
        {"manifest", "Manifest ellenőrzése", "Checking manifest"},
        {"graphics", "Grafikák letöltése", "Downloading graphics"},
        {"firmware", "Firmware ellenőrzése", "Checking firmware"},
        {"restarting", "Újraindítás", "Restarting"},
        {"complete", "Grafika naprakész", "Graphics up to date"},
        {"wifi.unavailable", "Nincs Wi-Fi kapcsolat", "No Wi-Fi connection"},
        {"clock.unavailable", "Az óra nincs beállítva", "Clock not set"},
        {"release.unavailable", "A kiadás nem érhető el", "Release unavailable"},
        {"firmware.required", "Új firmware szükséges", "New firmware required"},
        {"manifest.invalid", "Hibás manifest", "Invalid manifest"},
        {"graphics.failed", "Grafikafrissítési hiba", "Graphics update failed"},
        {"firmware.failed", "Firmware-frissítési hiba", "Firmware update failed"},
        {"memory", "Nincs elég memória", "Insufficient memory"}};
    for (unsigned i = 0; i < sizeof(texts) / sizeof(*texts); i++)
        if (!strcmp(phase, texts[i][0]))
            return texts[i][english ? 2 : 1];
    return english ? "Not measured" : "Nem mért";
}
static void number(cJSON *json, const char *key, bool valid, double value)
{
    if (valid)
        cJSON_AddNumberToObject(json, key, value);
    else
        cJSON_AddNullToObject(json, key);
}
static void flag(cJSON *json, const char *key, bool valid, bool value)
{
    if (valid)
        cJSON_AddBoolToObject(json, key, value);
    else
        cJSON_AddNullToObject(json, key);
}

cJSON *app_diagnostics_json(void)
{
    app_diagnostics_t data;
    app_diagnostics_capture(&data);
    cJSON *json = cJSON_CreateObject(), *wifi = cJSON_CreateObject(),
          *loxone = cJSON_CreateObject(), *update = cJSON_CreateObject(),
          *events = cJSON_CreateArray();
    if (!json || !wifi || !loxone || !update || !events)
    {
        cJSON_Delete(json);
        cJSON_Delete(wifi);
        cJSON_Delete(loxone);
        cJSON_Delete(update);
        cJSON_Delete(events);
        return NULL;
    }
    cJSON_AddBoolToObject(json, "measured", data.sampled);
    cJSON_AddBoolToObject(json, "cpuValid", data.cpu_valid);
    number(json, "uptime", data.sampled, data.uptime_seconds);
    number(json, "sampleAge", data.sampled, data.sample_age);
    cJSON_AddStringToObject(json, "firmware", data.firmware);
    flag(json, "clockValid", data.sampled, data.clock_valid);
    number(json, "cpu0", data.cpu_valid, data.cpu[0]);
    number(json, "cpu1", data.cpu_valid, data.cpu[1]);
    number(json, "internalFree", data.sampled, data.internal_free);
    number(json, "internalMinimum", data.sampled, data.internal_minimum);
    number(json, "internalTotal", data.sampled, data.internal_total);
    number(json, "psramFree", data.sampled, data.psram_free);
    number(json, "psramTotal", data.sampled, data.psram_total);
    number(json, "dmaFree", data.sampled, data.dma_free);
    number(json, "dmaLargest", data.sampled, data.dma_largest);
    number(json, "mainStackFree", data.sampled, data.main_stack_free);
    number(json, "taskCount", data.sampled, data.task_count);
    flag(json, "sdMounted", data.sampled, data.sd_mounted);
    flag(json, "storageRecovery", data.sampled, data.storage_recovery);
    cJSON_AddBoolToObject(json, "sdSpaceValid", data.sd_space_valid);
    number(json, "sdFree", data.sd_space_valid, data.sd_free);
    number(json, "sdTotal", data.sd_space_valid, data.sd_total);
    cJSON_AddNullToObject(json, "displayFps"); /* A célütem nem fizikai FPS-mérés. */
    flag(wifi, "connected", data.sampled, data.wifi_connected);
    number(wifi, "rssi", data.sampled && data.wifi_connected && data.rssi_valid, data.rssi);
    flag(wifi, "recovery", data.sampled, data.recovery_wifi);
    cJSON_AddStringToObject(wifi, "ip", data.ip);
    cJSON_AddStringToObject(wifi, "ssid", data.ssid);
    cJSON_AddStringToObject(wifi, "recoverySsid", data.recovery_ssid);
    flag(loxone, "connected", data.sampled, data.loxone_connected);
    number(loxone, "numericAge", data.sampled && data.numeric_age >= 0, data.numeric_age);
    number(loxone, "knownValues", data.sampled, data.known_values);
    flag(update, "busy", data.sampled, data.update_busy);
    flag(update, "success", data.sampled, data.update_success);
    cJSON_AddStringToObject(update, "phase", data.update_phase);
    cJSON_AddStringToObject(
        update, "phaseHu", app_diagnostics_phase_text(data.update_phase, false));
    cJSON_AddStringToObject(update, "phaseEn", app_diagnostics_phase_text(data.update_phase, true));
    cJSON_AddStringToObject(update, "version", data.update_version);
    number(update, "receivedBytes", data.sampled, data.update_received);
    for (size_t i = 0; i < data.event_count && i < APP_EVENT_LIMIT; i++)
    {
        const app_event_t *event = &data.events[i];
        cJSON *entry = cJSON_CreateObject();
        cJSON_AddNumberToObject(entry, "uptime", event->uptime_seconds);
        cJSON_AddNumberToObject(entry, "level", app_event_level(event->code));
        cJSON_AddStringToObject(entry, "code", app_event_code(event->code));
        cJSON_AddStringToObject(entry, "hu", app_event_text(event->code, false));
        cJSON_AddStringToObject(entry, "en", app_event_text(event->code, true));
        cJSON_AddItemToArray(events, entry);
    }
    cJSON_AddItemToObject(json, "wifi", wifi);
    cJSON_AddItemToObject(json, "loxone", loxone);
    cJSON_AddItemToObject(json, "update", update);
    cJSON_AddItemToObject(json, "events", events);
    return json;
}
