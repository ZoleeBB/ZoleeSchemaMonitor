#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SM_MAX_PAGES 10
#define SM_MAX_ELEMENTS 150
#define SM_UUID_SIZE 36

typedef struct
{
    bool configured;
    bool valid;
    double value;
} sm_signal_t;

typedef struct
{
    bool known;
    bool active;
    double speed;
} sm_activity_t;

typedef void (*sm_numeric_event_fn)(const char *uuid, double value, bool valid, void *context);

/* Explicit aktivitás mellett a teljesítmény csak a sebességet módosítja. */
sm_activity_t sm_activity(sm_signal_t activity, sm_signal_t power, double minimum, double maximum);
bool sm_power_range_valid(double minimum, double maximum);
bool sm_rotation_valid(unsigned rotation);
void sm_canvas_size(unsigned rotation, unsigned *width, unsigned *height);
bool sm_uuid_valid(const char *uuid);
void sm_uuid_string(const uint8_t raw[16], char uuid[SM_UUID_SIZE]);

/* Hibás csomagból egyetlen részleges eseményt sem adunk át a hívónak. */
bool sm_numeric_events(const uint8_t *data, size_t length, sm_numeric_event_fn callback, void *context);
