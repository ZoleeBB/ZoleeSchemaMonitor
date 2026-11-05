/* Valódi fájlműveletek és SHA-256, rövid olvasásokkal és tartósítási hibával. */
#include "../../main/app_assets.c"
#include <assert.h>

static const char *payloads[] = {"pump-data", "valve-data", "next-data"};
static char hashes[3][65];
static size_t offset, source_length;
static unsigned opens, closes;
static bool interrupted, extra, bad_hash, sync_failed;
static const char *source_bytes;
int __real_fsync(int descriptor);
int __wrap_fsync(int descriptor)
{
    return sync_failed ? -1 : __real_fsync(descriptor);
}
static int read_source(void *context, void *out, size_t capacity)
{
    (void)context;
    if (interrupted && offset >= 2)
        return -1;
    if (offset == source_length)
    {
        if (extra)
        {
            *(char *)out = '!';
            return 1;
        }
        return 0;
    }
    size_t count = source_length - offset;
    if (count > capacity)
        count = capacity;
    if (count > 2)
        count = 2;
    memcpy(out, source_bytes + offset, count);
    if (bad_hash)
        ((char *)out)[0] ^= 1;
    offset += count;
    return count;
}
static void close_source(void *context)
{
    (void)context;
    closes++;
}
static esp_err_t open_source(void *context, const char *sha, size_t size, app_asset_source_t *out)
{
    (void)context;
    for (unsigned i = 0; i < 3; i++)
        if (!strcmp(sha, hashes[i]))
        {
            opens++;
            offset = 0;
            source_bytes = payloads[i];
            source_length = strlen(source_bytes);
            assert(size == source_length);
            *out = (app_asset_source_t){.read = read_source, .close = close_source};
            return ESP_OK;
        }
    assert(false);
    return ESP_FAIL;
}
static cJSON *manifest(unsigned start, unsigned count)
{
    cJSON *json = cJSON_Parse(
        "{\"format\":\"zolee-assets\",\"version\":\"0.1.0\",\"assetAbi\":1,\"files\":[]}");
    cJSON *files = cJSON_GetObjectItemCaseSensitive(json, "files");
    for (unsigned i = start; i < start + count; i++)
    {
        cJSON *entry = cJSON_CreateObject();
        char path[64];
        snprintf(path, sizeof(path), "light/element-%u.png", i);
        cJSON_AddStringToObject(entry, "path", path);
        cJSON_AddStringToObject(entry, "sha256", hashes[i]);
        cJSON_AddNumberToObject(entry, "size", strlen(payloads[i]));
        cJSON_AddItemToArray(files, entry);
    }
    return json;
}
int main(void)
{
    for (unsigned i = 0; i < 3; i++)
    {
        unsigned char digest[32];
        assert(!mbedtls_sha256((const unsigned char *)payloads[i], strlen(payloads[i]), digest, 0));
        for (unsigned j = 0; j < 32; j++)
            snprintf(hashes[i] + j * 2, 3, "%02x", digest[j]);
    }
    /* A 300 feletti paletta két témával és SVG/PNG-vel is beleférjen. */
    cJSON *large = cJSON_Parse(
        "{\"format\":\"zolee-assets\",\"version\":\"0.1.0\",\"assetAbi\":1,\"files\":[]}");
    cJSON *large_files = cJSON_GetObjectItemCaseSensitive(large, "files");
    for (unsigned i = 0; i <= 2048; i++)
    {
        cJSON *entry = cJSON_CreateObject();
        char path[64];
        snprintf(path, sizeof(path), "light/type-%u.png", i);
        cJSON_AddStringToObject(entry, "path", path);
        cJSON_AddStringToObject(entry, "sha256", hashes[0]);
        cJSON_AddNumberToObject(entry, "size", strlen(payloads[0]));
        cJSON_AddItemToArray(large_files, entry);
        if (i == 1248 || i == 2047)
            assert(app_assets_manifest_valid(large));
    }
    assert(!app_assets_manifest_valid(large));
    cJSON_Delete(large);
    cJSON *first = manifest(0, 1), *second = manifest(0, 2), *third = manifest(2, 1);
    char future[APP_ASSET_ID_SIZE], later[APP_ASSET_ID_SIZE];
    snprintf(future, sizeof(future), "ZSM-ASSETS-1:%064d", 1);
    snprintf(later, sizeof(later), "ZSM-ASSETS-1:%064d", 2);
    assert(app_assets_identity_valid(app_assets_identity()));
    assert(!app_assets_identity_valid("../escape") && !app_assets_ready(future));
    assert(app_assets_prepare(app_assets_identity(), first, open_source, NULL) == ESP_OK);
    assert(opens == 1 && closes == 1 && app_assets_revision() == 1);
    assert(app_assets_ready(app_assets_identity()) && !app_assets_ready(future));
    assert(app_assets_prepare(app_assets_identity(), first, open_source, NULL) == ESP_OK &&
           opens == 1);
    interrupted = true;
    assert(app_assets_prepare(future, second, open_source, NULL) != ESP_OK);
    assert(app_assets_ready(app_assets_identity()) && !app_assets_ready(future) && opens == 2 &&
           closes == 2);
    interrupted = false;
    bad_hash = true;
    assert(app_assets_prepare(future, second, open_source, NULL) != ESP_OK &&
           !app_assets_ready(future));
    bad_hash = false;
    extra = true;
    assert(app_assets_prepare(future, second, open_source, NULL) != ESP_OK &&
           !app_assets_ready(future));
    extra = false;
    assert(app_assets_prepare(future, second, open_source, NULL) == ESP_OK && opens == 5 &&
           closes == 5);
    assert(app_assets_ready(future) && app_assets_ready(app_assets_identity()));
    assert(app_assets_revision() == 1); /* Más firmware készlete nem aktiválódik most. */
    assert(app_assets_prepare(future, first, open_source, NULL) == ESP_ERR_INVALID_STATE);
    sync_failed = true;
    assert(app_assets_prepare(later, third, open_source, NULL) != ESP_OK &&
           !app_assets_ready(later));
    sync_failed = false;
    assert(app_assets_ready(future) && app_assets_ready(app_assets_identity()));
    assert(app_assets_prepare(later, third, open_source, NULL) == ESP_OK &&
           app_assets_ready(later));
    char path[APP_ASSET_PATH_SIZE];
    cJSON *active = app_assets_manifest();
    assert(active);
    assert(app_assets_path(active, "light/element-0.png", path));
    assert(!app_assets_path(active, "../escape.png", path));
    FILE *file = fopen(path, "wb");
    assert(file);
    fputs("damaged", file);
    fclose(file);
    assert(!app_assets_ready(app_assets_identity()) && !app_assets_ready(future));
    unsigned before = opens;
    assert(app_assets_prepare(future, second, open_source, NULL) == ESP_OK && opens == before + 1);
    assert(app_assets_ready(app_assets_identity()));
    assert(app_assets_boot_ready());
    /* Sérült manifest javítása; csak az érvénytelen kötés írható újra. */
    char set_file[APP_ASSET_PATH_SIZE];
    snprintf(set_file, sizeof(set_file), APP_ASSET_ROOT "/sets/%s.json", later + 13);
    file = fopen(set_file, "wb");
    assert(file);
    fputs("{}", file);
    fclose(file);
    assert(!app_assets_ready(later));
    assert(app_assets_prepare(later, third, open_source, NULL) == ESP_OK &&
           app_assets_ready(later));
    char fourth[APP_ASSET_ID_SIZE];
    snprintf(fourth, sizeof(fourth), "ZSM-ASSETS-1:%064d", 3);
    sync_failed = true;
    assert(app_assets_prepare(fourth, first, open_source, NULL) != ESP_OK &&
           !app_assets_ready(fourth));
    sync_failed = false;
    assert(app_assets_ready(later) && app_assets_ready(future) &&
           app_assets_ready(app_assets_identity()));
    cJSON *bad = cJSON_Duplicate(first, true),
          *entry = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(bad, "files"), 0);
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(entry, "path"), "../escape.png");
    assert(!app_assets_manifest_valid(bad));
    cJSON_Delete(bad);
    bad = cJSON_Duplicate(first, true);
    entry = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(bad, "files"), 0);
    cJSON_AddItemToArray(cJSON_GetObjectItemCaseSensitive(bad, "files"),
                         cJSON_Duplicate(entry, true));
    assert(!app_assets_manifest_valid(bad));
    cJSON_Delete(bad);
    cJSON_Delete(active);
    cJSON_Delete(first);
    cJSON_Delete(second);
    cJSON_Delete(third);
    puts("Native assets: dedup, interruption, hash/length/fsync failures, repair, immutable sets "
         "and rollback selection OK");
}
