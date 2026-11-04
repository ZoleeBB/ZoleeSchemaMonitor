#include "schema_model.h"
#include <string.h>

/* A generált adattábla a firmware része; az SD nem vezethet be új típust. */
#include "schema_catalog.inc"

const sm_asset_t *sm_asset_find(const char *id)
{
    if (!id)
        return NULL;
    for (unsigned i = 0; i < sm_catalog_count; i++)
        if (!strcmp(id, sm_catalog[i].id))
            return &sm_catalog[i];
    return NULL;
}
