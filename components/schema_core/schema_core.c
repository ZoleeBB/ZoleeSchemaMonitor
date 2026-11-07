#include "schema_core.h"

#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

bool sm_spray_prepare(unsigned drop, double direction, sm_spray_ray_t *ray)
{
    if (!ray || !isfinite(direction) || drop >= SM_SPRAY_DROPS)
        return false;
    double angle = (fmod(direction, 360) + ((double)drop - 1.5) * 18) * 3.141592653589793 / 180;
    *ray = (sm_spray_ray_t){cos(angle), sin(angle)};
    return true;
}

bool sm_spray_sample(double phase, unsigned drop, const sm_spray_ray_t *ray, double *x, double *y)
{
    if (!ray || !x || !y || !isfinite(phase) || !isfinite(ray->x) || !isfinite(ray->y) ||
        drop >= SM_SPRAY_DROPS)
        return false;
    double t = phase - floor(phase);
    t = fmod(t + (double)drop / SM_SPRAY_DROPS, 1);
    double distance = .12 + .88 * t;
    *x = ray->x * distance;
    *y = ray->y * distance + .3 * t * t;
    return true;
}

bool sm_spray_point(double phase, unsigned drop, double direction, double *x, double *y)
{
    sm_spray_ray_t ray;
    return sm_spray_prepare(drop, direction, &ray) && sm_spray_sample(phase, drop, &ray, x, y);
}

static double clamp(double value)
{
    return value < 0 ? 0 : value > 1 ? 1 : value;
}

bool sm_power_range_valid(double minimum, double maximum)
{
    return isfinite(minimum) && isfinite(maximum) && maximum > minimum &&
           isfinite(maximum - minimum);
}

sm_position_t sm_position(sm_signal_t position, double minimum, double maximum, bool reverse)
{
    sm_position_t result = {0};
    if (!position.configured || !position.valid || !isfinite(position.value) ||
        !sm_power_range_valid(minimum, maximum))
        return result;

    /* Előbb korlátozunk: szélsőséges bemenetnél sem csordul túl a kivonás. */
    double fraction = position.value <= minimum ? 0 : position.value >= maximum ? 1 :
                      (position.value - minimum) / (maximum - minimum);
    result.known = true;
    result.percent = fraction * 100;
    /* Az irányfordítás a mutatót érinti, a mért nyitottságot nem. */
    result.angle = (reverse ? 1 - fraction : fraction) * 90 - 45;
    return result;
}

sm_activity_t sm_activity(sm_signal_t activity, sm_signal_t power, double minimum, double maximum)
{
    sm_activity_t result = {.known = true, .active = false, .speed = 0};
    bool power_valid =
        power.valid && isfinite(power.value) && sm_power_range_valid(minimum, maximum);

    if (activity.configured)
    {
        result.known = activity.valid && isfinite(activity.value);
        result.active = result.known && activity.value > 0;
    }
    else if (power.configured)
    {
        result.known = power_valid;
        result.active = result.known && power.value > minimum;
    }

    if (result.active)
    {
        /* Hibás opcionális teljesítmény nem írja felül az ismert aktivitást. */
        result.speed = power.configured && power_valid
                         ? clamp((power.value - minimum) / (maximum - minimum))
                         : 1;
        /* Aktív eszköznél nagyon kis értéknél is felismerhető a mozgás. */
        if (result.speed < 0.15)
            result.speed = 0.15;
    }
    return result;
}

bool sm_rotation_valid(unsigned rotation)
{
    return rotation == 0 || rotation == 90 || rotation == 180 || rotation == 270;
}

void sm_canvas_size(unsigned rotation, unsigned *width, unsigned *height)
{
    bool landscape = rotation == 90 || rotation == 270;
    *width = landscape ? 1280 : 800;
    *height = landscape ? 800 : 1280;
}

bool sm_uuid_valid(const char *uuid)
{
    if (!uuid || strlen(uuid) != 35)
        return false;
    for (unsigned i = 0; i < 35; i++)
    {
        if (i == 8 || i == 13 || i == 18)
        {
            if (uuid[i] != '-')
                return false;
        }
        else if (!isxdigit((unsigned char)uuid[i]))
            return false;
    }
    return true;
}

void sm_uuid_string(const uint8_t raw[16], char uuid[SM_UUID_SIZE])
{
    uint32_t first =
        (uint32_t)raw[0] | (uint32_t)raw[1] << 8 | (uint32_t)raw[2] << 16 | (uint32_t)raw[3] << 24;
    snprintf(uuid,
             SM_UUID_SIZE,
             "%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",
             (unsigned)first,
             raw[4] | raw[5] << 8,
             raw[6] | raw[7] << 8,
             raw[8],
             raw[9],
             raw[10],
             raw[11],
             raw[12],
             raw[13],
             raw[14],
             raw[15]);
}

bool sm_numeric_events(const uint8_t *data,
                       size_t length,
                       sm_numeric_event_fn callback,
                       void *context)
{
    if ((!data && length) || !callback || length % 24)
        return false;
    for (size_t position = 0; position < length; position += 24)
    {
        char uuid[SM_UUID_SIZE];
        sm_uuid_string(data + position, uuid);
        uint64_t bits = 0;
        for (unsigned i = 0; i < 8; i++)
            bits |= (uint64_t)data[position + 16 + i] << (8 * i);
        double value;
        memcpy(&value, &bits, sizeof(value));
        callback(uuid, value, isfinite(value), context);
    }
    return true;
}
