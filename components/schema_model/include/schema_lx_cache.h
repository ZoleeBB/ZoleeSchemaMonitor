#pragma once
#include "schema_loxone.h"
#define SM_LX_CACHE_SLOTS 8192
/* Külső zár védi; P4-en PSRAM-ban, tesztben a heapen tároljuk. */
typedef struct
{
    char uuid[SM_UUID_SIZE];
    double value;
    bool valid;
} sm_lx_entry_t;
typedef struct
{
    sm_lx_entry_t entries[SM_LX_CACHE_SLOTS];
    bool failed;
} sm_lx_cache_t;
void sm_lx_cache_clear(sm_lx_cache_t *cache);
bool sm_lx_cache_packet(sm_lx_cache_t *cache, const unsigned char *data, size_t length);
bool sm_lx_cache_read(const sm_lx_cache_t *cache, const char *uuid, double *value);
