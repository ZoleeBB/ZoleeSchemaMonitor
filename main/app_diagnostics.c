#include "app_diagnostics.h"

#include <stdatomic.h>
#include <string.h>
#include "esp_heap_caps.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static atomic_flag s_sampling = ATOMIC_FLAG_INIT;
static portMUX_TYPE s_lock = portMUX_INITIALIZER_UNLOCKED;
static uint64_t s_previous_idle[2];
static int64_t s_previous_time;
static unsigned s_cpu[2];
static bool s_cpu_valid;

void app_diagnostics_capture(app_diagnostics_t *out)
{
    memset(out, 0, sizeof(*out));
    int64_t now = esp_timer_get_time();
    if (!atomic_flag_test_and_set(&s_sampling))
    {
        if (!s_previous_time || now - s_previous_time >= 2000000)
        {
            unsigned cpu[2] = {0};
            uint64_t idle[2];
            for (unsigned core = 0; core < 2; core++)
            {
                TaskStatus_t task;
                vTaskGetInfo(xTaskGetIdleTaskHandleForCore(core), &task, pdFALSE, eInvalid);
                idle[core] = task.ulRunTimeCounter;
                if (s_previous_time)
                {
                    uint64_t elapsed = now - s_previous_time;
                    uint64_t delta = idle[core] - s_previous_idle[core];
                    cpu[core] = delta >= elapsed ? 0 : 100 - (unsigned)(delta * 100 / elapsed);
                }
            }
            portENTER_CRITICAL(&s_lock);
            memcpy(s_previous_idle, idle, sizeof(idle));
            memcpy(s_cpu, cpu, sizeof(cpu));
            s_cpu_valid = s_previous_time != 0;
            s_previous_time = now;
            portEXIT_CRITICAL(&s_lock);
        }
        atomic_flag_clear(&s_sampling);
    }
    portENTER_CRITICAL(&s_lock);
    memcpy(out->cpu, s_cpu, sizeof(s_cpu));
    out->cpu_valid = s_cpu_valid;
    portEXIT_CRITICAL(&s_lock);
    out->uptime_seconds = now / 1000000;
    out->internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    out->internal_minimum = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    out->psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    out->dma_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
}
