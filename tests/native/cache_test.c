#include "schema_lx_cache.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static void number(unsigned char *packet, double value)
{
    uint64_t bits;
    memcpy(&bits, &value, 8);
    for (unsigned i = 0; i < 8; i++)
        packet[16 + i] = bits >> (8 * i);
}
int main(void)
{
    sm_lx_cache_t *cache = calloc(1, sizeof(*cache));
    assert(cache);
    unsigned char packet[24] = {1};
    char uuid[SM_UUID_SIZE];
    sm_uuid_string(packet, uuid);
    double value = 123;
    assert(!sm_lx_cache_read(cache, uuid, &value) && value == 123);
    number(packet, 0);
    assert(sm_lx_cache_packet(cache, packet, 24));
    assert(sm_lx_cache_read(cache, uuid, &value) && value == 0);
    number(packet, 47.5);
    assert(sm_lx_cache_packet(cache, packet, 24));
    assert(sm_lx_cache_read(cache, uuid, &value) && value == 47.5);
    number(packet, NAN);
    assert(sm_lx_cache_packet(cache, packet, 24));
    assert(!sm_lx_cache_read(cache, uuid, &value));
    number(packet, 100);
    assert(sm_lx_cache_packet(cache, packet, 24));
    assert(!sm_lx_cache_packet(cache, packet, 23));
    assert(!sm_lx_cache_read(cache, uuid, &value));
    assert(sm_lx_cache_packet(cache, packet, 24));
    sm_lx_cache_clear(cache);
    assert(!sm_lx_cache_read(cache, uuid, &value));
    for (unsigned i = 0; i <= SM_LX_CACHE_SLOTS; i++)
    {
        for (unsigned n = 0; n < 4; n++)
            packet[n] = i >> (8 * n);
        bool ok = sm_lx_cache_packet(cache, packet, sizeof(packet));
        assert(ok == (i < SM_LX_CACHE_SLOTS));
    }
    sm_uuid_string(packet, uuid);
    assert(!sm_lx_cache_read(cache, uuid, &value));
    free(cache);
    puts("Loxone cache: unknown/zero, update, NaN, truncated packet and session reset OK");
}
