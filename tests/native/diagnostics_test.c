/* Az éles mintavétel, JSON és napló: a platformmérések szabályozott bemenetek. */
#include "app_diagnostics.h"
#include "app_network.h"
#include "app_loxone.h"
#include "esp_app_desc.h"
#include "esp_err.h"
#include "esp_heap_caps.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
static int64_t now;
static uint64_t idle[2];
static unsigned sd_reads, cpu_reads, stack_free = 4096, minimum_free = 12000;
static bool mounted = true, sd_ok = true, create_ok = true;
int64_t esp_timer_get_time(void)
{
    return now;
}
time_t __wrap_time(time_t *out)
{
    if (out)
        *out = 1735689600;
    return 1735689600;
}
TaskHandle_t xTaskGetIdleTaskHandleForCore(unsigned core)
{
    return (void *)(uintptr_t)(core + 1);
}
void vTaskGetInfo(TaskHandle_t task, TaskStatus_t *out, int stack, int state)
{
    (void)stack;
    (void)state;
    cpu_reads++;
    out->ulRunTimeCounter = idle[(uintptr_t)task - 1];
}
unsigned uxTaskGetStackHighWaterMark(TaskHandle_t task)
{
    assert(!task);
    return stack_free;
}
unsigned uxTaskGetNumberOfTasks(void)
{
    return 23;
}
size_t heap_caps_get_free_size(unsigned caps)
{
    return caps & MALLOC_CAP_DMA ? 32768 : caps & MALLOC_CAP_SPIRAM ? 30000000 : 16000;
}
size_t heap_caps_get_total_size(unsigned caps)
{
    return caps & MALLOC_CAP_SPIRAM ? 33554432 : 524288;
}
size_t heap_caps_get_minimum_free_size(unsigned caps)
{
    (void)caps;
    return minimum_free;
}
size_t heap_caps_get_largest_free_block(unsigned caps)
{
    (void)caps;
    return 8192;
}
bool app_store_ready(void)
{
    return mounted;
}
bool app_store_recovered(void)
{
    return true;
}
esp_err_t esp_vfs_fat_info(const char *path, uint64_t *total, uint64_t *free_bytes)
{
    assert(!strcmp(path, "/sdcard"));
    sd_reads++;
    *total = 64000000000ULL;
    *free_bytes = 16000000000ULL;
    return sd_ok ? ESP_OK : ESP_FAIL;
}
const esp_app_desc_t *esp_app_get_description(void)
{
    static const esp_app_desc_t description = {.version = "native-test"};
    return &description;
}
void app_network_status(app_network_status_t *out)
{
    memset(out, 0, sizeof(*out));
    out->connected = out->recovery = out->rssi_valid = true;
    out->rssi = -52;
    snprintf(out->ip, sizeof(out->ip), "192.168.2.123");
    snprintf(out->ssid, sizeof(out->ssid), "Wi-Fi <test>");
    snprintf(out->recovery_ssid, sizeof(out->recovery_ssid), "ZoleeSchema-Test");
    snprintf(out->recovery_password, sizeof(out->recovery_password), "MustNotLeak");
}
void app_loxone_diagnostics(app_loxone_diagnostics_t *out)
{
    *out = (app_loxone_diagnostics_t){.connected = true, .numeric_age = 0, .known_values = 42};
}
cJSON *app_release_status(void)
{
    return cJSON_Parse("{\"busy\":true,\"success\":false,\"phase\":\"graphics\",\"version\":\"0.1."
                       "0\",\"receivedBytes\":1024}");
}
int main(void)
{
    app_diagnostics_t data;
    app_diagnostics_capture(&data);
    assert(!data.sampled && !data.cpu_valid);
    cJSON *json = app_diagnostics_json();
    assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(json, "cpu0")));
    assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(json, "internalFree")));
    assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(json, "displayWindowUs")));
    assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(json, "displayFramesOver50Ms")));
    cJSON_Delete(json);
    app_events_add(APP_EVENT_BOOT);
    app_diagnostics_poll();
    app_diagnostics_capture(&data);
    assert(data.sampled && !data.cpu_valid && data.sd_space_valid && sd_reads == 1 &&
           cpu_reads == 2);
    assert(data.sd_total == 64000000000ULL && data.sd_free == 16000000000ULL);
    assert(data.main_stack_free == 4096 && data.task_count == 23 && data.dma_free == 32768);
    app_diagnostics_frame_completed(50000);
    app_diagnostics_frame_completed(50001);
    for (unsigned i = 0; i < 18; i++)
        app_diagnostics_frame_completed(11000 + i * 100);
    now = 1000000;
    app_diagnostics_poll();
    assert(cpu_reads == 2 && sd_reads == 1);
    json = app_diagnostics_json();
    assert(cJSON_GetObjectItemCaseSensitive(json, "displayFps")->valuedouble == 20.0);
    assert(cJSON_GetObjectItemCaseSensitive(json, "displayMaxRenderMs")->valuedouble == 50.001);
    assert(cJSON_GetObjectItemCaseSensitive(json, "displayWindowEndUs")->valueint == 1000000);
    assert(cJSON_GetObjectItemCaseSensitive(json, "displayWindowUs")->valueint == 1000000);
    assert(cJSON_GetObjectItemCaseSensitive(json, "displayFrames")->valueint == 20);
    assert(cJSON_GetObjectItemCaseSensitive(json, "displayFramesOver50Ms")->valueint == 1);
    cJSON_Delete(json);
    now = 2000000;
    idle[0] = 500000;
    idle[1] = 1500000;
    app_diagnostics_poll();
    app_diagnostics_capture(&data);
    assert(data.cpu_valid && data.cpu[0] == 75 && data.cpu[1] == 25 && cpu_reads == 4);
    assert(data.numeric_age == 0 && data.known_values == 42);
    for (unsigned i = 0; i < 20; i++)
    {
        json = app_diagnostics_json();
        char *text = cJSON_PrintUnformatted(json);
        assert(!strstr(text, "MustNotLeak") && !strstr(text, "password"));
        assert(cJSON_GetObjectItemCaseSensitive(json, "cpu0")->valueint == 75);
        assert(cJSON_GetObjectItemCaseSensitive(json, "displayFps")->valuedouble == 0);
        assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(json, "displayMaxRenderMs")));
        assert(cJSON_GetObjectItemCaseSensitive(json, "displayFrames")->valueint == 0);
        assert(cJSON_GetObjectItemCaseSensitive(json, "displayFramesOver50Ms")->valueint == 0);
        free(text);
        cJSON_Delete(json);
    }
    assert(cpu_reads == 4 && sd_reads == 1); /* A web csak a cache-t olvassa. */
    now = 7000000;
    app_diagnostics_capture(&data);
    assert(data.sample_age == 5);
    json = app_diagnostics_json();
    assert(cJSON_GetObjectItemCaseSensitive(json, "sampleAge")->valueint == 5);
    cJSON_Delete(json);
    assert(cpu_reads == 4 && sd_reads == 1);
    now = 10000000;
    sd_ok = false;
    app_diagnostics_poll();
    app_diagnostics_capture(&data);
    assert(!data.sd_space_valid && sd_reads == 2 && data.events[0].code == APP_EVENT_SD_SPACE);
    json = app_diagnostics_json();
    assert(cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(json, "sdFree")));
    cJSON_Delete(json);
    minimum_free = 4096;
    stack_free = 512;
    now = 20000000;
    app_diagnostics_poll();
    app_diagnostics_capture(&data);
    size_t warnings = data.event_count;
    assert(data.events[0].code == APP_EVENT_STACK_LOW &&
           data.events[1].code == APP_EVENT_MEMORY_LOW);
    now = 22000000;
    app_diagnostics_poll();
    app_diagnostics_capture(&data);
    assert(data.event_count == warnings);
    mounted = false;
    now = 23000000;
    unsigned previous_reads = sd_reads;
    app_diagnostics_poll();
    app_diagnostics_capture(&data);
    assert(!data.sd_mounted && !data.sd_space_valid && sd_reads == previous_reads);
    /* A túl nagy idle-delta nem okozhat negatív vagy 100% fölötti foglaltságot. */
    now = 25000000;
    idle[0] += 10000000;
    idle[1] += 10000000;
    app_diagnostics_poll();
    app_diagnostics_capture(&data);
    assert(data.cpu[0] == 0 && data.cpu[1] == 0);
    for (unsigned i = 0; i < 40; i++)
    {
        now++;
        app_events_add(i % APP_EVENT_COUNT);
    }
    app_event_t events[APP_EVENT_LIMIT];
    assert(app_events_snapshot(events) == 15);
    assert(events[0].code == 39 % APP_EVENT_COUNT && events[14].code == 25 % APP_EVENT_COUNT);
    app_events_add(APP_EVENT_COUNT);
    app_events_add((app_event_code_t)-1);
    app_events_snapshot(events);
    assert(events[0].code == 39 % APP_EVENT_COUNT);
    assert(!strcmp(app_event_text(APP_EVENT_WIFI_UP, true), "Wi-Fi connected"));
    create_ok = false;
    assert(app_diagnostics_start() == ESP_ERR_NO_MEM);
    create_ok = true;
    assert(app_diagnostics_start() == ESP_OK);
    assert(app_diagnostics_start() == ESP_ERR_INVALID_STATE);
    puts("Native diagnostics: CPU unknown/window/clamp, cached reads, SD cadence/failure, "
         "memory/stack warnings, ring log and secret-free JSON OK");
}

TaskHandle_t xTaskGetCurrentTaskHandle(void)
{
    return NULL;
}
int xTaskCreateWithCaps(void (*task)(void *),
                        const char *name,
                        size_t stack,
                        void *argument,
                        unsigned priority,
                        void *handle,
                        unsigned caps)
{
    (void)task;
    (void)name;
    (void)argument;
    (void)priority;
    (void)handle;
    assert(stack == 8192 && (caps & MALLOC_CAP_SPIRAM));
    return create_ok ? pdPASS : 0;
}
void vTaskDelay(unsigned ticks)
{
    (void)ticks;
}
