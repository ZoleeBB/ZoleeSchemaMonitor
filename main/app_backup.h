#pragma once
#include "schema_model.h"
#define APP_BACKUP_PLAIN_BYTES (SM_PROJECT_BYTES + 32768U)
#define APP_BACKUP_HTTP_BYTES (2U * APP_BACKUP_PLAIN_BYTES + 4096U)
/* Hordozható mentés: a fájl önálló jelszót használ, NVS-kulcsot nem exportál. */
bool app_backup_passphrase_valid(const char *passphrase);
cJSON *app_backup_seal(const cJSON *state, const char *passphrase);
cJSON *app_backup_open(const cJSON *envelope, const char *passphrase);
/* Csak saját, módosítható sztringeket tartalmazó JSON-fához használható. */
void app_backup_delete(cJSON *json);
