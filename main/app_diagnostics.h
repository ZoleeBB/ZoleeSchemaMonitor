#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint64_t uptime_seconds;
    unsigned cpu[2];
    bool cpu_valid;
    unsigned internal_free;
    unsigned internal_minimum;
    unsigned psram_free;
    unsigned dma_largest;
} app_diagnostics_t;

/* A testvérprojekt idle-időből számolt magonkénti mérésének adaptációja. */
void app_diagnostics_capture(app_diagnostics_t *out);
