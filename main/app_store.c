#include "app_store.h"
#include "app_events.h"
#include "bsp/esp-bsp.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "mbedtls/gcm.h"
#include "nvs.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifndef SM_STATE_DIRECTORY
#define SM_STATE_DIRECTORY "/sdcard/schema-state"
#endif
#define DIRECTORY SM_STATE_DIRECTORY
static SemaphoreHandle_t s_lock;
static cJSON *s_project, *s_settings;
static uint32_t s_generation, s_revision;
static unsigned char s_key[32];
static bool s_ready, s_recovered;

static char *hex(const unsigned char *data, size_t size)
{
    static const char digits[] = "0123456789abcdef";
    char *result = malloc(size * 2 + 1);
    if (result)
    {
        for (size_t i = 0; i < size; i++)
        {
            result[i * 2] = digits[data[i] >> 4];
            result[i * 2 + 1] = digits[data[i] & 15];
        }
        result[size * 2] = 0;
    }
    return result;
}

static int nibble(char c)
{
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : -1;
}

static bool unhex(const char *text, unsigned char *out, size_t size)
{
    if (strlen(text) != size * 2)
        return false;
    for (size_t i = 0; i < size; i++)
    {
        int a = nibble(text[i * 2]), b = nibble(text[i * 2 + 1]);
        if (a < 0 || b < 0)
            return false;
        out[i] = (unsigned char)(a * 16 + b);
    }
    return true;
}

static esp_err_t load_key(void)
{
    nvs_handle_t handle;
    esp_err_t result = nvs_open("schema", NVS_READWRITE, &handle);
    if (result != ESP_OK)
        return result;
    size_t size = sizeof(s_key);
    result = nvs_get_blob(handle, "storage_key", s_key, &size);
    if (result == ESP_ERR_NVS_NOT_FOUND)
    {
        esp_fill_random(s_key, sizeof(s_key));
        result = nvs_set_blob(handle, "storage_key", s_key, sizeof(s_key));
        if (result == ESP_OK)
            result = nvs_commit(handle);
    }
    else if (result == ESP_OK && size != sizeof(s_key))
        result = ESP_FAIL;
    nvs_close(handle);
    return result;
}

/* A jelszavak és a többi beállítás eszközhöz kötött AES-GCM borítékban van.
 * Az SD kártya másolata nem helyettesíti az eszköz NVS-kulcsának mentését. */
static cJSON *seal(const cJSON *settings)
{
    char *plain = cJSON_PrintUnformatted(settings);
    if (!plain)
        return NULL;
    size_t length = strlen(plain);
    unsigned char nonce[12], tag[16];
    unsigned char *cipher = malloc(length);
    esp_fill_random(nonce, sizeof(nonce));
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int result = cipher ? mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, s_key, 256) : -1;
    if (!result)
        result = mbedtls_gcm_crypt_and_tag(&context,
                                           MBEDTLS_GCM_ENCRYPT,
                                           length,
                                           nonce,
                                           sizeof(nonce),
                                           (const unsigned char *)"ZSM-settings-1",
                                           14,
                                           (unsigned char *)plain,
                                           cipher,
                                           sizeof(tag),
                                           tag);
    mbedtls_gcm_free(&context);
    memset(plain, 0, length);
    free(plain);
    cJSON *envelope = NULL;
    char *a = NULL, *b = NULL, *c = NULL;
    if (!result && (a = hex(nonce, sizeof(nonce))) && (b = hex(tag, sizeof(tag))) &&
        (c = hex(cipher, length)))
    {
        envelope = cJSON_CreateObject();
        if (envelope && (!cJSON_AddStringToObject(envelope, "nonce", a) ||
                         !cJSON_AddStringToObject(envelope, "tag", b) ||
                         !cJSON_AddStringToObject(envelope, "data", c)))
        {
            cJSON_Delete(envelope);
            envelope = NULL;
        }
    }
    free(a);
    free(b);
    free(c);
    free(cipher);
    return envelope;
}

static cJSON *unseal(const cJSON *envelope)
{
    unsigned char nonce[12], tag[16];
    const char *hexdata = sm_json_string(envelope, "data");
    size_t length = strlen(hexdata) / 2;
    if (!length || length > 16384 ||
        !unhex(sm_json_string(envelope, "nonce"), nonce, sizeof(nonce)) ||
        !unhex(sm_json_string(envelope, "tag"), tag, sizeof(tag)))
        return NULL;
    unsigned char *cipher = malloc(length), *plain = malloc(length + 1);
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int result = cipher && plain && unhex(hexdata, cipher, length)
                   ? mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, s_key, 256)
                   : -1;
    if (!result)
        result = mbedtls_gcm_auth_decrypt(&context,
                                          length,
                                          nonce,
                                          sizeof(nonce),
                                          (const unsigned char *)"ZSM-settings-1",
                                          14,
                                          tag,
                                          sizeof(tag),
                                          cipher,
                                          plain);
    cJSON *settings = !result ? sm_json_parse((char *)plain, length) : NULL;
    mbedtls_gcm_free(&context);
    if (plain)
        memset(plain, 0, length + 1);
    free(plain);
    free(cipher);
    return settings;
}

static bool persist(const cJSON *project, const cJSON *settings, uint32_t revision)
{
    if (!s_ready || s_generation == UINT32_MAX)
        return false;
    cJSON *root = cJSON_CreateObject(), *encrypted = seal(settings),
          *copy = cJSON_Duplicate(project, true);
    bool ok = root && encrypted && copy && cJSON_AddNumberToObject(root, "version", 1) &&
              cJSON_AddNumberToObject(root, "revision", revision);
    if (ok)
    {
        ok = cJSON_AddItemToObject(root, "project", copy);
        if (ok)
            copy = NULL;
        if (ok)
            ok = cJSON_AddItemToObject(root, "settings", encrypted);
        if (ok)
            encrypted = NULL;
        char *json = ok ? cJSON_PrintUnformatted(root) : NULL;
        ok = json && sm_journal_write(DIRECTORY, json, s_generation + 1);
        free(json);
    }
    cJSON_Delete(copy);
    cJSON_Delete(encrypted);
    cJSON_Delete(root);
    if (ok)
        s_generation++;
    return ok;
}

esp_err_t app_store_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    s_project = sm_project_defaults();
    s_settings = sm_settings_defaults();
    if (!s_lock || !s_project || !s_settings)
        return ESP_ERR_NO_MEM;
    esp_err_t result = load_key();
    if (result != ESP_OK)
        return result;
    result = bsp_sdcard_mount();
    if (result != ESP_OK)
        return result;
    struct stat info;
    if (stat(DIRECTORY, &info) != 0 && mkdir(DIRECTORY, 0700) != 0)
        return ESP_FAIL;
    s_ready = true;
    uint32_t best = 0;
    for (unsigned slot = 0; slot < 2; slot++)
    {
        uint32_t generation = 0;
        char *json = sm_journal_read_slot(DIRECTORY, slot, &generation);
        if (!json)
        {
            char path[128];
            snprintf(path, sizeof(path), DIRECTORY "/state-%u.bin", slot);
            if (stat(path, &info) == 0)
                s_recovered = true;
            continue;
        }
        if (generation > s_generation)
            s_generation = generation;
        cJSON *root = sm_json_parse(json, strlen(json));
        free(json);
        cJSON *project = cJSON_GetObjectItemCaseSensitive(root, "project");
        cJSON *settings = unseal(cJSON_GetObjectItemCaseSensitive(root, "settings"));
        double revision = sm_json_number(root, "revision", -1);
        char error[SM_ERROR_SIZE];
        bool valid = sm_json_number(root, "version", 0) == 1 && revision >= 0 &&
                     revision <= UINT32_MAX && revision == (uint32_t)revision &&
                     sm_project_validate(project, error) && settings &&
                     sm_settings_validate(settings, error);
        if (valid && generation > best)
        {
            cJSON *copy = cJSON_Duplicate(project, true);
            if (copy)
            {
                cJSON_Delete(s_project);
                cJSON_Delete(s_settings);
                s_project = copy;
                s_settings = settings;
                settings = NULL;
                s_revision = (uint32_t)revision;
                best = generation;
            }
        }
        else if (!valid)
            s_recovered = true;
        cJSON_Delete(settings);
        cJSON_Delete(root);
    }
    /* A következő írás mindig a betöltött, érvényes példány másik slotjába megy.
     * Egy újabb, de szemantikailag hibás rekord nem fordíthatja meg ezt. */
    if (best)
        s_generation = best;
    /* Érvényes mentés híján nem írjuk felül automatikusan a sérült kártyát. */
    return ESP_OK;
}

bool app_store_ready(void)
{
    return s_ready;
}
bool app_store_recovered(void)
{
    return s_recovered;
}

cJSON *app_store_project(uint32_t *revision)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    cJSON *copy = cJSON_Duplicate(s_project, true);
    if (revision)
        *revision = s_revision;
    xSemaphoreGive(s_lock);
    return copy;
}

uint32_t app_store_revision(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    uint32_t revision = s_revision;
    xSemaphoreGive(s_lock);
    return revision;
}

cJSON *app_store_settings(bool public_view)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    cJSON *copy = cJSON_Duplicate(s_settings, true);
    xSemaphoreGive(s_lock);
    if (copy && public_view)
    {
        const char *sections[] = {"wifi", "loxone"};
        for (unsigned i = 0; i < 2; i++)
        {
            cJSON *section = cJSON_GetObjectItemCaseSensitive(copy, sections[i]);
            bool has = *sm_json_string(section, "password") != 0;
            cJSON_DeleteItemFromObjectCaseSensitive(section, "password");
            cJSON_AddBoolToObject(section, "hasPassword", has);
        }
    }
    return copy;
}

esp_err_t app_store_save_project(const cJSON *project,
                                 uint32_t expected,
                                 uint32_t *revision,
                                 char error[SM_ERROR_SIZE])
{
    if (!sm_project_validate(project, error))
        return ESP_ERR_INVALID_ARG;
    char *encoded = cJSON_PrintUnformatted(project);
    bool size_ok = encoded && strlen(encoded) <= SM_PROJECT_BYTES;
    free(encoded);
    if (!size_ok)
    {
        snprintf(error, SM_ERROR_SIZE, "project.size");
        return ESP_ERR_INVALID_SIZE;
    }
    cJSON *copy = cJSON_Duplicate(project, true);
    if (!copy)
        return ESP_ERR_NO_MEM;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    esp_err_t result = ESP_ERR_INVALID_STATE;
    if (expected != s_revision)
        snprintf(error, SM_ERROR_SIZE, "revision.conflict");
    else if (s_revision == UINT32_MAX || !persist(copy, s_settings, s_revision + 1))
    {
        result = ESP_FAIL;
        snprintf(error, SM_ERROR_SIZE, "storage.write");
        app_events_add(APP_EVENT_SD_WRITE);
    }
    else
    {
        cJSON_Delete(s_project);
        s_project = copy;
        copy = NULL;
        *revision = ++s_revision;
        result = ESP_OK;
    }
    xSemaphoreGive(s_lock);
    cJSON_Delete(copy);
    return result;
}

static bool replace(cJSON *object, const char *name, const cJSON *value)
{
    cJSON *copy = cJSON_Duplicate(value, true);
    if (!copy)
        return false;
    bool ok = cJSON_ReplaceItemInObjectCaseSensitive(object, name, copy);
    if (!ok)
        cJSON_Delete(copy);
    return ok;
}

esp_err_t app_store_save_settings(const cJSON *patch, bool loxone, char error[SM_ERROR_SIZE])
{
    if (!cJSON_IsObject(patch))
        return ESP_ERR_INVALID_ARG;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    cJSON *next = cJSON_Duplicate(s_settings, true);
    bool ok = next != NULL;
    if (ok && loxone)
    {
        ok = replace(next, "loxone", patch);
    }
    else if (ok)
    {
        const cJSON *entry;
        cJSON_ArrayForEach(entry, patch)
        {
            if (!entry->string ||
                (strcmp(entry->string, "project") && strcmp(entry->string, "display") &&
                 strcmp(entry->string, "wifi")) ||
                !replace(next, entry->string, entry))
            {
                ok = false;
                break;
            }
        }
    }
    const char *section_name = loxone ? "loxone" : "wifi";
    cJSON *section = cJSON_GetObjectItemCaseSensitive(next, section_name);
    if (ok && cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(section, "password")))
        ok = replace(section,
                     "password",
                     cJSON_GetObjectItemCaseSensitive(
                         cJSON_GetObjectItemCaseSensitive(s_settings, section_name), "password"));
    esp_err_t result = ESP_ERR_INVALID_ARG;
    if (!ok)
        snprintf(error, SM_ERROR_SIZE, "settings.fields");
    else if (sm_settings_validate(next, error))
    {
        if (!persist(s_project, next, s_revision))
        {
            result = ESP_FAIL;
            snprintf(error, SM_ERROR_SIZE, "storage.write");
            app_events_add(APP_EVENT_SD_WRITE);
        }
        else
        {
            cJSON_Delete(s_settings);
            s_settings = next;
            next = NULL;
            result = ESP_OK;
        }
    }
    cJSON_Delete(next);
    xSemaphoreGive(s_lock);
    return result;
}
