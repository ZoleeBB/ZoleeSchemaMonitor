#pragma once

#include "cJSON.h"
#include "schema_core.h"

#define SM_PROJECT_BYTES 1048576U
#define SM_ERROR_SIZE 160
#define SM_MAX_POINTS 64

typedef struct
{
    float x, y;
} sm_point_t;
typedef struct
{
    const char *id;
    const char *name_hu;
    const char *name_en;
    const char *family;
    unsigned port_count;
    sm_point_t ports[34];
    unsigned animation; /* 0: nincs, 1: rotor, 2: vízpermet, 3: üzemjelző, 4: állásmutató. */
    float animation_x, animation_y, animation_radius;
    float rotor_scale_x, rotor_scale_y;
    unsigned extra_rotor_count;
    struct
    {
        float x, y, radius;
    } extra_rotors[7];
} sm_asset_t;

extern const sm_asset_t sm_catalog[];
extern const unsigned sm_catalog_count;
const sm_asset_t *sm_asset_find(const char *id);
/* Pontosan egy UTF-8 JSON dokumentum; a beágyazott NUL nem csonkíthat mezőt. */
cJSON *sm_json_parse(const char *data, size_t length);
/* Titkosított mentés külső JSON-borítékához, változatlan mélység/UTF-8 szabályokkal. */
cJSON *sm_json_parse_limit(const char *data, size_t length, size_t limit);
const char *sm_json_string(const cJSON *object, const char *key);
double sm_json_number(const cJSON *object, const char *key, double fallback);
/* Ugyanazok a nyilvános korlátok, mint a web/model.js modellben. */
bool sm_project_validate(const cJSON *project, char error[SM_ERROR_SIZE]);
bool sm_binding_validate(const cJSON *binding);
bool sm_settings_validate(const cJSON *settings, char error[SM_ERROR_SIZE]);
cJSON *sm_settings_defaults(void);
cJSON *sm_project_defaults(void);

/* A megjelenítő és a webes előnézet ugyanazt az átalakítást használja. */
typedef struct
{
    char uuid[SM_UUID_SIZE];
    double scale, offset;
} sm_binding_t;
bool sm_binding_parse(const cJSON *json, sm_binding_t *binding);
sm_signal_t sm_binding_read(const sm_binding_t *binding, bool valid, double raw);

/* POSIX/SD egyaránt: CRC-vel védett, két példányos tranzakciós projektállapot. */
char *sm_journal_read_slot(const char *directory, unsigned slot, uint32_t *generation);
bool sm_journal_write(const char *directory, const char *json, uint32_t generation);
