#pragma once
#include "schema_model.h"
#define SM_LX_STRUCTURE_BYTES (512U * 1024U)
#define SM_LX_CATALOG_MAX 2048
/* Teljes WS-üzeneteket fogad. A WS-fragmentáció összeállítása a transzporté.
 * A callbackben kapott adat csak a hívás idejéig használható. Nincs foglalás. */
typedef struct
{
    bool pending;
    unsigned type;
    size_t length;
} sm_lx_frame_t;
typedef bool (*sm_lx_message_fn)(unsigned type,
                                 const unsigned char *data,
                                 size_t length,
                                 void *context);
bool sm_lx_message(sm_lx_frame_t *frame,
                   const unsigned char *data,
                   size_t length,
                   sm_lx_message_fn callback,
                   void *context);
/* Csak olvasható állapotok, parancscélok és hitelesítő adatok nélkül. */
cJSON *sm_lx_catalog(const char *json, size_t length);
