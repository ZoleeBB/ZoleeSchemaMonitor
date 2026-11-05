/* Tartalomcímzett SD-tár. Nincs globális aktív mutató: a firmware saját
 * azonosítója választ készletet, ezért a bootloader rollback grafikát is vált. */
#define _POSIX_C_SOURCE 200809L
#include "app_assets.h"
#include "mbedtls/sha256.h"
#include "ota_identity.inc"
#include <errno.h>
#include <math.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef APP_ASSET_ROOT
#define APP_ASSET_ROOT "/sdcard/graphics"
#endif
static atomic_uint s_revision;
uint32_t app_assets_revision(void)
{
    return atomic_load(&s_revision);
}
static atomic_flag s_preparing = ATOMIC_FLAG_INIT;

const char *app_assets_identity(void)
{
    return SCHEMA_OTA_IDENTITY;
}

static bool sha_valid(const char *sha)
{
    if (!sha || strlen(sha) != 64)
        return false;
    return strspn(sha, "0123456789abcdef") == 64;
}

bool app_assets_identity_valid(const char *identity)
{
    return identity && strlen(identity) == APP_ASSET_ID_SIZE - 1 &&
           !strncmp(identity, "ZSM-ASSETS-1:", 13) && sha_valid(identity + 13);
}

static bool name_valid(const char *name)
{
    if (!name || !*name || strlen(name) >= 96 || *name == '/' || strstr(name, "..") ||
        strstr(name, "//"))
        return false;
    size_t allowed =
        strspn(name, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_/-");
    return allowed > 0 && (!strcmp(name + allowed, ".svg") || !strcmp(name + allowed, ".png") ||
                           !strcmp(name + allowed, ".json"));
}

bool app_assets_manifest_valid(const cJSON *manifest)
{
    if (!cJSON_IsObject(manifest) || cJSON_GetArraySize(manifest) != 4 ||
        strcmp(sm_json_string(manifest, "format"), "zolee-assets") ||
        sm_json_number(manifest, "assetAbi", 0) != 1)
        return false;
    const char *version = sm_json_string(manifest, "version");
    if (!*version || strlen(version) > 64 ||
        strspn(version, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") !=
            strlen(version))
        return false;
    const cJSON *files = cJSON_GetObjectItemCaseSensitive(manifest, "files");
    int count = cJSON_GetArraySize(files);
    if (!cJSON_IsArray(files) || count < 1 || count > 1024)
        return false;
    double total = 0;
    const cJSON *entry;
    cJSON_ArrayForEach(entry, files)
    {
        const char *name = sm_json_string(entry, "path"), *sha = sm_json_string(entry, "sha256");
        double size = sm_json_number(entry, "size", 0);
        if (!cJSON_IsObject(entry) || cJSON_GetArraySize(entry) != 3 || !name_valid(name) ||
            !sha_valid(sha) || !isfinite(size) || size < 1 || size > 4194304 || size != floor(size))
            return false;
        total += size;
        for (const cJSON *previous = files->child; previous != entry; previous = previous->next)
            if (!strcmp(name, sm_json_string(previous, "path")) ||
                (!strcmp(sha, sm_json_string(previous, "sha256")) &&
                 size != sm_json_number(previous, "size", 0)))
                return false;
    }
    return total <= 134217728;
}

static cJSON *read_json(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return NULL;
    char *bytes = NULL;
    cJSON *json = NULL;
    if (!fseek(file, 0, SEEK_END))
    {
        long size = ftell(file);
        if (size > 0 && size <= APP_ASSET_MANIFEST_BYTES && !fseek(file, 0, SEEK_SET))
        {
            bytes = malloc(size);
            if (bytes && fread(bytes, 1, size, file) == (size_t)size)
                json = sm_json_parse(bytes, size);
        }
    }
    fclose(file);
    free(bytes);
    return json;
}

static bool valid_file(const char *path, const char *sha, size_t expected)
{
    struct stat info;
    if (stat(path, &info) || !S_ISREG(info.st_mode) || info.st_size != (off_t)expected)
        return false;
    FILE *file = fopen(path, "rb");
    unsigned char *buffer = file ? malloc(4096) : NULL;
    if (!buffer)
    {
        if (file)
            fclose(file);
        return false;
    }
    mbedtls_sha256_context hash;
    mbedtls_sha256_init(&hash);
    bool ok = mbedtls_sha256_starts(&hash, 0) == 0;
    size_t used = 0, size;
    while (ok && (size = fread(buffer, 1, 4096, file)) > 0)
    {
        used += size;
        ok = used <= expected && mbedtls_sha256_update(&hash, buffer, size) == 0;
    }
    unsigned char digest[32];
    char hex[65];
    ok = ok && !ferror(file) && used == expected && mbedtls_sha256_finish(&hash, digest) == 0;
    if (ok)
    {
        for (unsigned i = 0; i < 32; i++)
            snprintf(hex + i * 2, 3, "%02x", digest[i]);
        ok = !strcmp(hex, sha);
    }
    mbedtls_sha256_free(&hash);
    fclose(file);
    free(buffer);
    return ok;
}

static void blob_path(const char *sha, char path[APP_ASSET_PATH_SIZE])
{
    snprintf(path, APP_ASSET_PATH_SIZE, APP_ASSET_ROOT "/blobs/%s", sha);
}
static void set_path(const char *identity, char path[APP_ASSET_PATH_SIZE])
{
    snprintf(path, APP_ASSET_PATH_SIZE, APP_ASSET_ROOT "/sets/%s.json", identity + 13);
}

static bool all_present(const cJSON *manifest)
{
    if (!app_assets_manifest_valid(manifest))
        return false;
    const cJSON *entry;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(manifest, "files"))
    {
        char path[APP_ASSET_PATH_SIZE];
        blob_path(sm_json_string(entry, "sha256"), path);
        if (!valid_file(path, sm_json_string(entry, "sha256"), sm_json_number(entry, "size", 0)))
            return false;
    }
    return true;
}

static bool legacy_identity_matches(void)
{
    char identity[APP_ASSET_ID_SIZE];
    FILE *file = fopen(APP_ASSET_ROOT "/identity.txt", "rb");
    if (!file)
        return false;
    size_t size = fread(identity, 1, sizeof(identity), file);
    bool matching = size == strlen(SCHEMA_OTA_IDENTITY) && !ferror(file) && fgetc(file) == EOF &&
                    !memcmp(identity, SCHEMA_OTA_IDENTITY, size);
    fclose(file);
    return matching;
}

bool app_assets_ready(const char *identity)
{
    if (!app_assets_identity_valid(identity))
        return false;
    char path[APP_ASSET_PATH_SIZE];
    set_path(identity, path);
    cJSON *manifest = read_json(path);
    if (!manifest && !strcmp(identity, SCHEMA_OTA_IDENTITY) && legacy_identity_matches())
        manifest = read_json(APP_ASSET_ROOT "/manifest.json");
    bool ready = all_present(manifest);
    cJSON_Delete(manifest);
    return ready;
}

bool app_assets_boot_ready(void)
{
    /* A teljes ellenőrzés bootonként egyszer sikerülhet; hibát újra vizsgálunk. */
    static bool ready;
    if (ready)
        return true;
    ready = app_assets_ready(SCHEMA_OTA_IDENTITY);
    return ready;
}

cJSON *app_assets_manifest(void)
{
    char path[APP_ASSET_PATH_SIZE];
    set_path(SCHEMA_OTA_IDENTITY, path);
    cJSON *json = read_json(path);
    if (json)
        return json;
    return legacy_identity_matches() ? read_json(APP_ASSET_ROOT "/manifest.json") : NULL;
}

bool app_assets_path(const cJSON *manifest, const char *relative, char path[APP_ASSET_PATH_SIZE])
{
    if (!name_valid(relative))
        return false;
    const cJSON *entry;
    cJSON_ArrayForEach(entry,
                       cJSON_GetObjectItemCaseSensitive(
                           manifest, "files")) if (!strcmp(relative, sm_json_string(entry, "path")))
    {
        const char *sha = sm_json_string(entry, "sha256");
        double size = sm_json_number(entry, "size", 0);
        if (!sha_valid(sha) || size < 1 || size > 4194304 || size != floor(size))
            return false;
        blob_path(sha, path);
        if (valid_file(path, sha, size))
            return true;
        snprintf(path, APP_ASSET_PATH_SIZE, APP_ASSET_ROOT "/%s", relative);
        return valid_file(path, sha, size);
    }
    return false;
}

static bool directory(const char *path)
{
    struct stat info;
    return mkdir(path, 0775) == 0 ||
           (errno == EEXIST && !stat(path, &info) && S_ISDIR(info.st_mode));
}

static esp_err_t
copy_blob(const char *sha, size_t size, app_assets_open_fn open, void *context, bool *changed)
{
    *changed = false;
    char path[APP_ASSET_PATH_SIZE], temporary[APP_ASSET_PATH_SIZE + 6];
    blob_path(sha, path);
    if (valid_file(path, sha, size))
        return ESP_OK;
    if (!open)
        return ESP_ERR_INVALID_STATE;
    app_asset_source_t source = {0};
    esp_err_t result = open(context, sha, size, &source);
    if (result != ESP_OK)
        return result;
    if (!source.read || !source.close)
    {
        if (source.close)
            source.close(source.context);
        return ESP_ERR_INVALID_ARG;
    }
    snprintf(temporary, sizeof(temporary), "%s.part", path);
    FILE *file = fopen(temporary, "wb");
    unsigned char *buffer = file ? malloc(4096) : NULL;
    result = buffer ? ESP_OK : ESP_ERR_NO_MEM;
    size_t used = 0;
    while (result == ESP_OK && used < size)
    {
        size_t wanted = size - used > 4096 ? 4096 : size - used;
        int count = source.read(source.context, buffer, wanted);
        if (count <= 0 || (size_t)count > wanted || fwrite(buffer, 1, count, file) != (size_t)count)
            result = ESP_FAIL;
        else
            used += count;
    }
    /* A HTTP-forrás lezárása előtt a túl hosszú törzset is elutasítjuk. */
    if (result == ESP_OK && source.read(source.context, buffer, 1) != 0)
        result = ESP_ERR_INVALID_SIZE;
    if (file)
    {
        if (result == ESP_OK && (fflush(file) || fsync(fileno(file))))
            result = ESP_FAIL;
        if (fclose(file))
            result = ESP_FAIL;
    }
    free(buffer);
    source.close(source.context);
    if (result == ESP_OK && !valid_file(temporary, sha, size))
        result = ESP_ERR_INVALID_ARG;
    if (result == ESP_OK)
    {
        /* FAT nem ír felül átnevezéssel. Csak eleve hibás blob törölhető. */
        if (remove(path) && errno != ENOENT)
            result = ESP_FAIL;
        if (result == ESP_OK && rename(temporary, path))
            result = ESP_FAIL;
    }
    if (result != ESP_OK)
        remove(temporary);
    else
        *changed = true;
    return result;
}

static esp_err_t
prepare(const char *identity, const cJSON *manifest, app_assets_open_fn open, void *context)
{
    if (!app_assets_identity_valid(identity) || !app_assets_manifest_valid(manifest))
        return ESP_ERR_INVALID_ARG;
    if (!directory(APP_ASSET_ROOT) || !directory(APP_ASSET_ROOT "/blobs") ||
        !directory(APP_ASSET_ROOT "/sets"))
        return ESP_FAIL;
    char path[APP_ASSET_PATH_SIZE], temporary[APP_ASSET_PATH_SIZE + 6];
    set_path(identity, path);
    cJSON *previous = read_json(path);
    bool previous_valid = app_assets_manifest_valid(previous);
    bool same = previous_valid && cJSON_Compare(cJSON_GetObjectItemCaseSensitive(previous, "files"),
                                                cJSON_GetObjectItemCaseSensitive(manifest, "files"),
                                                true);
    cJSON_Delete(previous);
    if (previous_valid && !same)
        return ESP_ERR_INVALID_STATE;
    bool changed = false;
    const cJSON *entry;
    cJSON_ArrayForEach(entry, cJSON_GetObjectItemCaseSensitive(manifest, "files"))
    {
        bool downloaded;
        esp_err_t result = copy_blob(sm_json_string(entry, "sha256"),
                                     sm_json_number(entry, "size", 0),
                                     open,
                                     context,
                                     &downloaded);
        if (result != ESP_OK)
            return result;
        changed |= downloaded;
    }
    if (same)
    {
        if (changed && !strcmp(identity, SCHEMA_OTA_IDENTITY))
            atomic_fetch_add(&s_revision, 1);
        return ESP_OK;
    }
    char *raw = cJSON_PrintUnformatted(manifest);
    if (!raw)
        return ESP_ERR_NO_MEM;
    snprintf(temporary, sizeof(temporary), "%s.part", path);
    FILE *file = fopen(temporary, "wb");
    bool ok = file && fwrite(raw, 1, strlen(raw), file) == strlen(raw) && !fflush(file) &&
              !fsync(fileno(file));
    if (file && fclose(file))
        ok = false;
    free(raw);
    /* Csak sérült vagy hiányzó kötést javítunk, a tartós új példány után. */
    if (ok && remove(path) && errno != ENOENT)
        ok = false;
    if (ok)
        ok = rename(temporary, path) == 0;
    if (!ok)
        remove(temporary);
    if (ok && !strcmp(identity, SCHEMA_OTA_IDENTITY))
        atomic_fetch_add(&s_revision, 1);
    return ok ? ESP_OK : ESP_FAIL;
}

esp_err_t app_assets_prepare(const char *identity,
                             const cJSON *manifest,
                             app_assets_open_fn open,
                             void *context)
{
    if (atomic_flag_test_and_set(&s_preparing))
        return ESP_ERR_INVALID_STATE;
    esp_err_t result = prepare(identity, manifest, open, context);
    atomic_flag_clear(&s_preparing);
    return result;
}
