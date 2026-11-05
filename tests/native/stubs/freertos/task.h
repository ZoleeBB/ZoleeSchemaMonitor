#pragma once
#include <stdint.h>
typedef void *TaskHandle_t;
typedef struct
{
    uint64_t ulRunTimeCounter;
} TaskStatus_t;
enum
{
    eInvalid
};
void vTaskGetInfo(TaskHandle_t task, TaskStatus_t *out, int stack, int state);
TaskHandle_t xTaskGetIdleTaskHandleForCore(unsigned core);
unsigned uxTaskGetStackHighWaterMark(TaskHandle_t task);
unsigned uxTaskGetNumberOfTasks(void);

TaskHandle_t xTaskGetCurrentTaskHandle(void);
