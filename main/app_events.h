#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#define APP_EVENT_LIMIT 15
/* Csak előre definiált események: jelszó, URL, token és felhasználói szöveg
 * nem kerülhet a naplóba. A napló RAM-ban él, újraindításkor kiürül. */
typedef enum
{
    APP_EVENT_BOOT,
    APP_EVENT_WIFI_UP,
    APP_EVENT_WIFI_DOWN,
    APP_EVENT_RECOVERY,
    APP_EVENT_SD_READY,
    APP_EVENT_SD_MISSING,
    APP_EVENT_SD_RECOVERED,
    APP_EVENT_SD_WRITE,
    APP_EVENT_SD_SPACE,
    APP_EVENT_LOXONE_UP,
    APP_EVENT_LOXONE_DOWN,
    APP_EVENT_UPDATE_START,
    APP_EVENT_UPDATE_DONE,
    APP_EVENT_UPDATE_FAILED,
    APP_EVENT_MEMORY_LOW,
    APP_EVENT_STACK_LOW,
    APP_EVENT_COUNT
} app_event_code_t;
typedef struct
{
    uint64_t uptime_seconds;
    app_event_code_t code;
} app_event_t;
void app_events_add(app_event_code_t code);
size_t app_events_snapshot(app_event_t events[APP_EVENT_LIMIT]);
const char *app_event_code(app_event_code_t code);
const char *app_event_text(app_event_code_t code, bool english);
unsigned app_event_level(app_event_code_t code);
