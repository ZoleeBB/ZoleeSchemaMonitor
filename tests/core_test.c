#include "schema_core.h"
#include <assert.h>
#include <math.h>
#include <string.h>

static unsigned events;
static void received(const char *uuid, double value, bool valid, void *context)
{
    (void)context;
    assert(!strcmp(uuid, "12345678-1234-5678-0102030405060708"));
    assert(valid && value == 42.5);
    events++;
}

int main(void)
{
    sm_signal_t none = {0}, on = {true, true, 1}, off = {true, true, 0};
    sm_signal_t unknown = {true, false, 90}, half = {true, true, 50};
    assert(sm_activity(none, half, 0, 100).active);
    assert(sm_activity(none, half, 0, 100).speed == 0.5);
    assert(!sm_activity(off, half, 0, 100).active);
    assert(!sm_activity(unknown, half, 0, 100).known);
    assert(sm_activity(on, unknown, 0, 100).active);
    assert(!sm_activity(none, off, 0, 100).active);
    assert(!sm_activity(none, half, 50, 100).active);
    assert(sm_activity(none, half, 0, 40).speed == 1);
    assert(!sm_power_range_valid(100, 0));
    assert(!sm_power_range_valid(NAN, 100));
    /* A mutató adathiánya nem azonos a nulla százalékos állással. */
    assert(!sm_position(none, 0, 100, false).known);
    assert(!sm_position(unknown, 0, 100, false).known);
    assert(!sm_position(half, 100, 100, false).known);
    assert(!sm_position(half, -INFINITY, 100, false).known);
    assert(!sm_position((sm_signal_t){true, true, NAN}, 0, 100, false).known);
    sm_position_t position = sm_position(off, 0, 100, false);
    assert(position.known && position.percent == 0 && position.angle == -45);
    position = sm_position(half, 0, 100, false);
    assert(position.known && position.percent == 50 && position.angle == 0);
    position = sm_position((sm_signal_t){true, true, 200}, 0, 100, true);
    assert(position.percent == 100 && position.angle == -45);
    position = sm_position((sm_signal_t){true, true, -200}, 0, 100, true);
    assert(position.percent == 0 && position.angle == 45);
    position = sm_position((sm_signal_t){true, true, 6}, 2, 10, false);
    assert(position.percent == 50 && position.angle == 0);
    unsigned w, h;
    sm_canvas_size(0, &w, &h);
    assert(w == 800 && h == 1280);
    sm_canvas_size(270, &w, &h);
    assert(w == 1280 && h == 800);
    assert(sm_rotation_valid(180) && !sm_rotation_valid(45));
    assert(sm_uuid_valid("12345678-1234-5678-0102030405060708"));
    assert(!sm_uuid_valid("12345678-1234-5678-0102-030405060708"));
    uint8_t packet[25] = {0x78, 0x56, 0x34, 0x12, 0x34, 0x12, 0x78, 0x56, 1, 2, 3, 4, 5, 6, 7, 8};
    double value = 42.5;
    uint64_t bits;
    memcpy(&bits, &value, 8);
    for (unsigned i = 0; i < 8; i++)
        packet[16 + i] = bits >> (8 * i);
    assert(!sm_numeric_events(packet, 25, received, NULL) && events == 0);
    assert(sm_numeric_events(packet, 24, received, NULL) && events == 1);
    return 0;
}
