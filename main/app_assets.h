#pragma once
#include "app_ota.h"
#include "schema_model.h"

#define APP_ASSET_ID_SIZE 78
#define APP_ASSET_PATH_SIZE 192
#define APP_ASSET_MANIFEST_BYTES 262144

/* Egy forrás olvasóját pontosan egyszer zárjuk le, hiba esetén is. */
typedef struct
{
    app_ota_read_fn read;
    void (*close)(void *context);
    void *context;
} app_asset_source_t;
typedef esp_err_t (*app_assets_open_fn)(void *context,
                                        const char *sha,
                                        size_t size,
                                        app_asset_source_t *source);

bool app_assets_identity_valid(const char *identity);
bool app_assets_manifest_valid(const cJSON *manifest);
/* Csak teljes készlethez készül tartós kötés; a korábbi készletek megmaradnak. */
esp_err_t app_assets_prepare(const char *identity,
                             const cJSON *manifest,
                             app_assets_open_fn open,
                             void *context);
bool app_assets_ready(const char *identity);
bool app_assets_boot_ready(void);
cJSON *app_assets_manifest(void);
/* A web és a natív rajzoló is ugyanahhoz a változatlan blobhoz jut. */
bool app_assets_path(const cJSON *manifest, const char *relative, char path[APP_ASSET_PATH_SIZE]);
const char *app_assets_identity(void);
uint32_t app_assets_revision(void);
