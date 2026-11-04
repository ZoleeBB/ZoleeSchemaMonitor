#pragma once
#include "schema_model.h"
/* Az adatolvasó csak RAM-ból dolgozhat, hálózat/SD nélkül; kapcsolati hibánál
 * valid=false eredményt ad. Az installálás a rajzoló task indulása előtt történik. */
typedef bool (*app_scene_read_fn)(const char *uuid, double *value, void *context);
void app_scene_set_reader(app_scene_read_fn reader, void *context);
void app_scene_update(unsigned elapsed_ms);
/* A hívó nem tarthat kijelzőzárat: a frissítés előbb SD-t olvas. */
void app_scene_poll(void);
