#pragma once
#include <stddef.h>
#define pdPASS 1
#define pdMS_TO_TICKS(value) (value)
int xTaskCreateWithCaps(void (*task)(void *),
                        const char *name,
                        size_t stack,
                        void *argument,
                        unsigned priority,
                        void *handle,
                        unsigned caps);
void vTaskDeleteWithCaps(void *task);
void vTaskDelay(unsigned ticks);
