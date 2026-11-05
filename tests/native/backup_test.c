/* Valódi Mbed TLS: ismert PBKDF2-vektor és hitelesített, hordozható mentés. */
#include "../../main/app_backup.c"
#include <assert.h>
#include <stdio.h>
int main(void)
{
    unsigned char salt[16], key[32];
    for (size_t i = 0; i < sizeof(salt); i++)
        salt[i] = i;
    assert(derive("Backup test 123!", salt, key) == 0);
    char *derived = hex(key, sizeof(key));
    /* Függetlenül, Python hashlib.pbkdf2_hmac függvénnyel számolt referencia. */
    assert(!strcmp(derived, "d5408a64758aa3dd82e48b98f34566f9ae713b21374818b483e25c174e17c0c4"));
    free(derived);
    assert(!app_backup_passphrase_valid("short"));
    assert(app_backup_passphrase_valid("árvíztűrő1234"));
    const char *fixture = "{\"password\":\"Private fixture only!\"}";
    cJSON *state = sm_json_parse(fixture, strlen(fixture));
    assert(state);
    const char *password = "Mentési jelszó 123!";
    cJSON *a = app_backup_seal(state, password), *b = app_backup_seal(state, password);
    assert(a && b && strcmp(sm_json_string(a, "salt"), sm_json_string(b, "salt")));
    assert(strcmp(sm_json_string(a, "nonce"), sm_json_string(b, "nonce")));
    char *serialized = cJSON_PrintUnformatted(a);
    assert(!strstr(serialized, "Private fixture") && !strstr(serialized, password));
    free(serialized);
    cJSON *opened = app_backup_open(a, password);
    assert(opened && cJSON_Compare(opened, state, true));
    app_backup_delete(opened);
    assert(!app_backup_open(a, "Wrong password 123!"));
    const char *fields[] = {"salt", "nonce", "tag", "data"};
    for (size_t i = 0; i < 4; i++)
    {
        cJSON *bad = cJSON_Duplicate(a, true);
        char *value = cJSON_GetObjectItemCaseSensitive(bad, fields[i])->valuestring;
        value[0] = value[0] == '0' ? '1' : '0';
        assert(!app_backup_open(bad, password));
        cJSON_Delete(bad);
    }
    cJSON_SetNumberValue(cJSON_GetObjectItemCaseSensitive(b, "iterations"), 600001);
    assert(!app_backup_open(b, password));
    cJSON_AddBoolToObject(a, "extra", true);
    assert(!app_backup_open(a, password));
    cJSON_Delete(a);
    cJSON_Delete(b);
    app_backup_delete(state);
    /* A nagy titkosított boríték csak a kifejezett limitű parserrel fogadható. */
    size_t length = SM_PROJECT_BYTES + 40000;
    char *large = malloc(length);
    assert(large);
    memset(large, ' ', length);
    memcpy(large, "{}", 2);
    assert(!sm_json_parse(large, length));
    cJSON *parsed = sm_json_parse_limit(large, length, APP_BACKUP_HTTP_BYTES);
    assert(parsed);
    cJSON_Delete(parsed);
    free(large);
    puts("Native backup: independent PBKDF2 vector, randomness, encryption, tamper/password "
         "rejection and parser limits OK");
}
