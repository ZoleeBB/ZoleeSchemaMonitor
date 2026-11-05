#pragma once
#include <pthread.h>
#define portMAX_DELAY 0xffffffffU
#define pdFALSE 0
typedef pthread_mutex_t portMUX_TYPE;
#define portMUX_INITIALIZER_UNLOCKED PTHREAD_MUTEX_INITIALIZER
#define portENTER_CRITICAL(lock) pthread_mutex_lock(lock)
#define portEXIT_CRITICAL(lock) pthread_mutex_unlock(lock)
