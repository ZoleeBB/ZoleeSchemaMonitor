/* Helyi indítású, önálló kiadásfrissítő. A kliens nem adhat meg letöltési URL-t. */
#include "app_release.h"
#include "app_assets.h"
#include "app_ota.h"
#include "app_network.h"
#include "esp_app_desc.h"
#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "freertos/idf_additions.h"
#include "mbedtls/sha256.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define RELEASE_BASE "https://github.com/ZoleeBB/ZoleeSchemaMonitor/releases/"
static SemaphoreHandle_t s_lock;
static bool s_busy, s_success;
static char s_phase[32] = "idle", s_version[65];
static uint32_t s_received;

typedef struct
{
    esp_http_client_handle_t client;
    char location[2048];
    bool bad_location;
} download_t;

static void phase(const char *text)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    snprintf(s_phase, sizeof(s_phase), "%s", text);
    xSemaphoreGive(s_lock);
}

bool app_release_url_allowed(const char *url)
{
    const char *prefixes[] = {"https://github.com/",
                              "https://release-assets.githubusercontent.com/",
                              "https://objects.githubusercontent.com/"};
    if (!url || strchr(url, '\r') || strchr(url, '\n'))
        return false;
    for (unsigned i = 0; i < sizeof(prefixes) / sizeof(*prefixes); i++)
        if (!strncmp(url, prefixes[i], strlen(prefixes[i])))
            return true;
    return false;
}

static esp_err_t event(esp_http_client_event_t *event)
{
    download_t *source = event->user_data;
    if (event->event_id == HTTP_EVENT_ON_HEADER && !strcasecmp(event->header_key, "Location"))
    {
        if (strlen(event->header_value) >= sizeof(source->location))
            source->bad_location = true;
        else
            snprintf(source->location, sizeof(source->location), "%s", event->header_value);
    }
    return ESP_OK;
}

static void download_close(void *context)
{
    download_t *source = context;
    esp_http_client_cleanup(source->client);
    free(source);
}

static int download_read(void *context, void *buffer, size_t capacity)
{
    download_t *source = context;
    int count = esp_http_client_read(source->client, buffer, capacity);
    if (count == 0 && !esp_http_client_is_complete_data_received(source->client))
        return -1;
    if (count > 0)
    {
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_received += count;
        xSemaphoreGive(s_lock);
    }
    return count;
}

static esp_err_t download_open(const char *url, size_t expected, app_asset_source_t *out)
{
    if (!app_release_url_allowed(url))
        return ESP_ERR_INVALID_ARG;
    download_t *source = calloc(1, sizeof(*source));
    if (!source)
        return ESP_ERR_NO_MEM;
    esp_http_client_config_t config = {.url = url,
                                       .timeout_ms = 15000,
                                       .crt_bundle_attach = esp_crt_bundle_attach,
                                       .disable_auto_redirect = true,
                                       .buffer_size = 4096,
                                       .buffer_size_tx = 1024,
                                       .event_handler = event,
                                       .user_data = source};
    source->client = esp_http_client_init(&config);
    if (!source->client)
    {
        free(source);
        return ESP_ERR_NO_MEM;
    }
    for (unsigned attempt = 0; attempt < 6; attempt++)
    {
        source->location[0] = 0;
        source->bad_location = false;
        if (esp_http_client_open(source->client, 0) != ESP_OK)
            break;
        int64_t size = esp_http_client_fetch_headers(source->client);
        int status = esp_http_client_get_status_code(source->client);
        if (status == 200 && (size >= 0 || esp_http_client_is_chunked_response(source->client)) &&
            (!expected || esp_http_client_is_chunked_response(source->client) ||
             size == (int64_t)expected))
        {
            *out = (app_asset_source_t){
                .read = download_read, .close = download_close, .context = source};
            return ESP_OK;
        }
        if (source->bad_location ||
            !(status == 301 || status == 302 || status == 303 || status == 307 || status == 308) ||
            !app_release_url_allowed(source->location))
            break;
        esp_http_client_close(source->client);
        if (esp_http_client_set_url(source->client, source->location) != ESP_OK)
            break;
    }
    download_close(source);
    return ESP_FAIL;
}

static bool sha_valid(const char *sha)
{
    return strlen(sha) == 64 && strspn(sha, "0123456789abcdef") == 64;
}

bool app_release_descriptor_valid(const cJSON *descriptor)
{
    const char *version = sm_json_string(descriptor, "version");
    if (!cJSON_IsObject(descriptor) || cJSON_GetArraySize(descriptor) != 5 ||
        strcmp(sm_json_string(descriptor, "format"), "zolee-update") || !*version ||
        strlen(version) > 64 ||
        strspn(version, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") !=
            strlen(version) ||
        !app_assets_identity_valid(sm_json_string(descriptor, "identity")))
        return false;
    const char *keys[] = {"manifest", "firmware"};
    for (unsigned i = 0; i < 2; i++)
    {
        const cJSON *entry = cJSON_GetObjectItemCaseSensitive(descriptor, keys[i]);
        double size = sm_json_number(entry, "size", 0);
        if (!cJSON_IsObject(entry) || cJSON_GetArraySize(entry) != 2 || !isfinite(size) ||
            size < 1 || size != floor(size) || size > (i ? 8388608 : APP_ASSET_MANIFEST_BYTES) ||
            !sha_valid(sm_json_string(entry, "sha256")))
            return false;
    }
    return true;
}

static cJSON *json_download(const char *url, size_t limit, const cJSON *expected)
{
    app_asset_source_t source;
    size_t wanted = expected ? sm_json_number(expected, "size", 0) : 0;
    if (download_open(url, wanted, &source) != ESP_OK)
        return NULL;
    unsigned char *bytes = malloc(limit + 1);
    size_t used = 0;
    int count = -1;
    if (bytes)
        while (used <= limit &&
               (count = source.read(source.context, bytes + used, limit + 1 - used)) > 0)
            used += count;
    source.close(source.context);
    cJSON *json = NULL;
    bool valid = bytes && count == 0 && used > 0 && used <= limit && (!expected || used == wanted);
    if (valid && expected)
    {
        unsigned char hash[32];
        char hex[65];
        valid = mbedtls_sha256(bytes, used, hash, 0) == 0;
        if (valid)
        {
            for (unsigned i = 0; i < 32; i++)
                snprintf(hex + i * 2, 3, "%02x", hash[i]);
            valid = !strcmp(hex, sm_json_string(expected, "sha256"));
        }
    }
    if (valid)
        json = sm_json_parse((const char *)bytes, used);
    free(bytes);
    return json;
}

static esp_err_t asset_open(void *context, const char *sha, size_t size, app_asset_source_t *source)
{
    char url[256];
    snprintf(url, sizeof(url), RELEASE_BASE "download/v%s/%s", (const char *)context, sha);
    return download_open(url, size, source);
}

typedef struct
{
    app_asset_source_t source;
    mbedtls_sha256_context hash;
    size_t used, expected;
    const char *sha, *identity;
    char received_identity[APP_ASSET_ID_SIZE];
    bool valid;
} firmware_t;

static int firmware_read(void *context, void *buffer, size_t capacity)
{
    firmware_t *firmware = context;
    int count = firmware->source.read(firmware->source.context, buffer, capacity);
    if (count > 0)
    {
        for (size_t i = 0; i < (size_t)count; i++)
            if (firmware->used + i >= 288 && firmware->used + i < 288 + APP_ASSET_ID_SIZE)
                firmware->received_identity[firmware->used + i - 288] = ((char *)buffer)[i];
        firmware->used += count;
        if (mbedtls_sha256_update(&firmware->hash, buffer, count))
            firmware->valid = false;
    }
    return count;
}

static bool firmware_verify(void *context)
{
    firmware_t *firmware = context;
    unsigned char hash[32];
    char hex[65], extra;
    if (!firmware->valid || firmware->used != firmware->expected ||
        firmware->source.read(firmware->source.context, &extra, 1) != 0 ||
        memcmp(firmware->received_identity, firmware->identity, APP_ASSET_ID_SIZE) ||
        mbedtls_sha256_finish(&firmware->hash, hash))
        return false;
    for (unsigned i = 0; i < 32; i++)
        snprintf(hex + i * 2, 3, "%02x", hash[i]);
    return !strcmp(hex, firmware->sha);
}

static esp_err_t firmware_download(const char *url, const cJSON *descriptor)
{
    const cJSON *entry = cJSON_GetObjectItemCaseSensitive(descriptor, "firmware");
    firmware_t firmware = {.expected = sm_json_number(entry, "size", 0),
                           .sha = sm_json_string(entry, "sha256"),
                           .identity = sm_json_string(descriptor, "identity"),
                           .valid = true};
    if (download_open(url, firmware.expected, &firmware.source) != ESP_OK)
        return ESP_FAIL;
    mbedtls_sha256_init(&firmware.hash);
    firmware.valid = mbedtls_sha256_starts(&firmware.hash, 0) == 0;
    esp_err_t result =
        app_ota_upload_verified(firmware.expected, firmware_read, &firmware, firmware_verify);
    mbedtls_sha256_free(&firmware.hash);
    firmware.source.close(firmware.source.context);
    return result;
}

static void worker(void *argument)
{
    bool firmware = (uintptr_t)argument != 0, restart = false;
    cJSON *descriptor = NULL, *manifest = NULL;
    esp_err_t result = ESP_FAIL;
    app_network_status_t network;
    app_network_status(&network);
    if (!network.connected)
    {
        phase("wifi.unavailable");
        goto done;
    }
    if (time(NULL) < 1704067200)
        phase("clock");
    for (unsigned i = 0; i < 30 && time(NULL) < 1704067200; i++)
        vTaskDelay(pdMS_TO_TICKS(1000));
    if (time(NULL) < 1704067200)
    {
        phase("clock.unavailable");
        goto done;
    }
    descriptor = json_download(RELEASE_BASE "latest/download/update.json", 8192, NULL);
    if (!app_release_descriptor_valid(descriptor))
    {
        phase("release.unavailable");
        goto done;
    }
    const char *version = sm_json_string(descriptor, "version"),
               *identity = sm_json_string(descriptor, "identity");
    xSemaphoreTake(s_lock, portMAX_DELAY);
    snprintf(s_version, sizeof(s_version), "%s", version);
    xSemaphoreGive(s_lock);
    if (!firmware && strcmp(identity, app_assets_identity()))
    {
        phase("firmware.required");
        goto done;
    }
    char url[256];
    snprintf(url, sizeof(url), RELEASE_BASE "download/v%s/asset-manifest.json", version);
    phase("manifest");
    manifest = json_download(
        url, APP_ASSET_MANIFEST_BYTES, cJSON_GetObjectItemCaseSensitive(descriptor, "manifest"));
    if (!app_assets_manifest_valid(manifest) ||
        strcmp(sm_json_string(manifest, "version"), version))
    {
        phase("manifest.invalid");
        goto done;
    }
    phase("graphics");
    result = app_assets_prepare(identity, manifest, asset_open, (void *)version);
    if (result != ESP_OK)
    {
        phase("graphics.failed");
        goto done;
    }
    if (firmware && strcmp(version, esp_app_get_description()->version))
    {
        phase("firmware");
        snprintf(url, sizeof(url), RELEASE_BASE "download/v%s/ZoleeSchemaMonitor.bin", version);
        result = firmware_download(url, descriptor);
        if (result != ESP_OK)
        {
            phase("firmware.failed");
            goto done;
        }
        restart = true;
        phase("restarting");
    }
    else
        phase("complete");
done:
    cJSON_Delete(manifest);
    cJSON_Delete(descriptor);
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_success = result == ESP_OK;
    s_busy = s_success && restart;
    xSemaphoreGive(s_lock);
    if (result == ESP_OK && restart)
        app_network_restart_later();
    vTaskDeleteWithCaps(NULL);
}

esp_err_t app_release_init(void)
{
    if (!s_lock)
        s_lock = xSemaphoreCreateMutex();
    return s_lock ? ESP_OK : ESP_ERR_NO_MEM;
}

esp_err_t app_release_start(bool firmware)
{
    if (!s_lock)
        return ESP_ERR_INVALID_STATE;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_busy)
    {
        xSemaphoreGive(s_lock);
        return ESP_ERR_INVALID_STATE;
    }
    s_busy = true;
    s_success = false;
    s_received = 0;
    s_version[0] = 0;
    snprintf(s_phase, sizeof(s_phase), "release");
    xSemaphoreGive(s_lock);
    if (xTaskCreateWithCaps(worker,
                            "schema_update",
                            16384,
                            (void *)(uintptr_t)firmware,
                            3,
                            NULL,
                            MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT) == pdPASS)
        return ESP_OK;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_busy = false;
    snprintf(s_phase, sizeof(s_phase), "memory");
    xSemaphoreGive(s_lock);
    return ESP_ERR_NO_MEM;
}

cJSON *app_release_status(void)
{
    cJSON *json = cJSON_CreateObject();
    if (s_lock)
        xSemaphoreTake(s_lock, portMAX_DELAY);
    cJSON_AddBoolToObject(json, "busy", s_busy);
    cJSON_AddBoolToObject(json, "success", s_success);
    cJSON_AddStringToObject(json, "phase", s_phase);
    cJSON_AddStringToObject(json, "version", s_version);
    cJSON_AddNumberToObject(json, "receivedBytes", s_received);
    if (s_lock)
        xSemaphoreGive(s_lock);
    return json;
}
