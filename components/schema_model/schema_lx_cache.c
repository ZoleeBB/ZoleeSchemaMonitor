#include "schema_lx_cache.h"
#include <string.h>
static unsigned hash(const char *uuid)
{
    uint32_t value = 2166136261U;
    while (*uuid)
        value = (value ^ (unsigned char)*uuid++) * 16777619U;
    return value % SM_LX_CACHE_SLOTS;
}
void sm_lx_cache_clear(sm_lx_cache_t *cache)
{
    memset(cache, 0, sizeof(*cache));
}
static void event(const char *uuid, double value, bool valid, void *context)
{
    sm_lx_cache_t *cache = context;
    unsigned start = hash(uuid);
    for (unsigned offset = 0; offset < SM_LX_CACHE_SLOTS; offset++)
    {
        sm_lx_entry_t *entry = &cache->entries[(start + offset) % SM_LX_CACHE_SLOTS];
        if (!entry->uuid[0] || !strcmp(entry->uuid, uuid))
        {
            memcpy(entry->uuid, uuid, SM_UUID_SIZE);
            entry->value = value;
            entry->valid = valid;
            return;
        }
    }
    cache->failed = true;
}
bool sm_lx_cache_packet(sm_lx_cache_t *cache, const unsigned char *data, size_t length)
{
    if (!sm_numeric_events(data, length, event, cache) || cache->failed)
    {
        sm_lx_cache_clear(cache);
        return false;
    }
    return true;
}
bool sm_lx_cache_read(const sm_lx_cache_t *cache, const char *uuid, double *value)
{
    if (!sm_uuid_valid(uuid))
        return false;
    unsigned start = hash(uuid);
    for (unsigned offset = 0; offset < SM_LX_CACHE_SLOTS; offset++)
    {
        const sm_lx_entry_t *entry = &cache->entries[(start + offset) % SM_LX_CACHE_SLOTS];
        if (!entry->uuid[0])
            return false;
        if (!strcmp(entry->uuid, uuid))
        {
            if (entry->valid)
                *value = entry->value;
            return entry->valid;
        }
    }
    return false;
}
