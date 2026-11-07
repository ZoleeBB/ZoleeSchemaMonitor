#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define SM_MAX_PAGES 10
#define SM_MAX_ELEMENTS 150
#define SM_UUID_SIZE 36
#define SM_SPRAY_DROPS 4

typedef struct
{
    double x, y;
} sm_spray_ray_t;
/* Az állandó szögeket csak jelenetépítéskor kell kiszámolni. */
bool sm_spray_prepare(unsigned drop, double direction, sm_spray_ray_t *ray);
bool sm_spray_sample(double phase, unsigned drop, const sm_spray_ray_t *ray, double *x, double *y);
/* Permetcsepp relatív pályája; a megjelenítő méretezi és forgatja. Nincs memóriafoglalás. */
bool sm_spray_point(double phase, unsigned drop, double direction, double *x, double *y);

typedef struct
{
    bool configured;
    bool valid;
    double value;
} sm_signal_t;

/* Egységes kijelzett szám: 0–6 tizedes, negatív nulla nélkül, nagy értéknél 6 számjegy. */
bool sm_value_text(sm_signal_t signal, unsigned decimals, const char *unit, char *output, size_t capacity);

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
bool sm_numeric_events(const uint8_t *data,
                       size_t length,
                       sm_numeric_event_fn callback,
                       void *context);

/* A szelepállás nem aktivitás: adathiánynál a mutatót el kell rejteni. */
typedef struct
{
    bool known;
    double percent;
    double angle;
} sm_position_t;

sm_position_t sm_position(sm_signal_t position, double minimum, double maximum, bool reverse);
