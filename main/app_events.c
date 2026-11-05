/* A testvérprojekt 15 soros eseménynaplójának rögzített kódú adaptációja. */
#include "app_events.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static app_event_t s_events[APP_EVENT_LIMIT];
static size_t s_head, s_count;
void app_events_add(app_event_code_t code)
{
    if (code >= APP_EVENT_COUNT || code < 0)
        return;
    uint64_t now = esp_timer_get_time() / 1000000;
    portENTER_CRITICAL(&s_lock);
    s_events[s_head] = (app_event_t){.uptime_seconds = now, .code = code};
    s_head = (s_head + 1) % APP_EVENT_LIMIT;
    if (s_count < APP_EVENT_LIMIT)
        s_count++;
    portEXIT_CRITICAL(&s_lock);
}
size_t app_events_snapshot(app_event_t events[APP_EVENT_LIMIT])
{
    portENTER_CRITICAL(&s_lock);
    size_t count = s_count;
    for (size_t i = 0; i < count; i++)
        events[i] = s_events[(s_head + APP_EVENT_LIMIT - 1 - i) % APP_EVENT_LIMIT];
    portEXIT_CRITICAL(&s_lock);
    return count;
}
