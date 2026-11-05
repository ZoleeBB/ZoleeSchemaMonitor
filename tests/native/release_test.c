/* Éles kiadásfrissítő és SD-tár, szimulált HTTPS/socket és OTA flash végpont. */
#include "../../main/app_release.c"
#include <assert.h>

struct fake_http
{
    esp_http_client_config_t config;
    char url[2048];
    const unsigned char *bytes;
    size_t size, offset;
    int status;
    bool chunked;
};
static char *descriptor_bytes, *manifest_bytes;
static char current_version[32] = "native-test";
static time_t clock_value = 1735689600;
static unsigned clock_waits;
time_t __wrap_time(time_t *out)
{
    if (out)
        *out = clock_value;
    return clock_value;
}
const esp_app_desc_t *esp_app_get_description(void)
{
    static esp_app_desc_t descriptor;
    snprintf(descriptor.version, sizeof(descriptor.version), "%s", current_version);
    return &descriptor;
}
static unsigned char firmware_bytes[1000];
static const unsigned char asset_bytes[] = "verified-pump";
static char asset_sha[65];
static bool external_redirect, asset_corrupt, body_cut, task_deferred, network_up = true;
static bool restarted;
static unsigned opens, cleaned, boot_targets, firmware_calls, asset_calls;
static void (*deferred_task)(void *);
static void *deferred_argument;

static void hash_bytes(const void *bytes, size_t size, char sha[65])
{
    unsigned char hash[32];
    assert(!mbedtls_sha256(bytes, size, hash, 0));
    for (unsigned i = 0; i < 32; i++)
        snprintf(sha + i * 2, 3, "%02x", hash[i]);
}
esp_err_t esp_crt_bundle_attach(void *config)
{
    (void)config;
    return ESP_OK;
}
void app_network_status(app_network_status_t *status)
{
    memset(status, 0, sizeof(*status));
    status->connected = network_up;
}
void app_network_restart_later(void)
{
    restarted = true;
}
int xTaskCreateWithCaps(void (*task)(void *),
                        const char *name,
                        size_t stack,
                        void *argument,
                        unsigned priority,
                        void *handle,
                        unsigned caps)
{
    (void)name;
    (void)priority;
    (void)handle;
    assert(stack >= 16384 && (caps & MALLOC_CAP_INTERNAL));
    if (task_deferred)
    {
        deferred_task = task;
        deferred_argument = argument;
    }
    else
        task(argument);
    return pdPASS;
}
void vTaskDeleteWithCaps(void *task)
{
    assert(!task);
}
void vTaskDelay(unsigned ticks)
{
    (void)ticks;
    clock_waits++;
}
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config)
{
    assert(config->disable_auto_redirect && config->crt_bundle_attach);
    struct fake_http *client = calloc(1, sizeof(*client));
    assert(client);
    client->config = *config;
    snprintf(client->url, sizeof(client->url), "%s", config->url);
    opens++;
    return client;
}
esp_err_t esp_http_client_open(esp_http_client_handle_t client, int size)
{
    assert(size == 0);
    client->offset = 0;
    client->status = 200;
    client->chunked = false;
    const char *file = strrchr(client->url, '/') + 1;
    if (strstr(client->url, "/latest/download/"))
    {
        client->status = 302;
        client->size = 0;
    }
    else if (!strcmp(file, "update.json"))
    {
        client->bytes = (void *)descriptor_bytes;
        client->size = strlen(descriptor_bytes);
    }
    else if (!strcmp(file, "asset-manifest.json"))
    {
        client->bytes = (void *)manifest_bytes;
        client->size = strlen(manifest_bytes);
        client->chunked = true;
    }
    else if (!strcmp(file, "ZoleeSchemaMonitor.bin"))
    {
        client->bytes = firmware_bytes;
        client->size = sizeof(firmware_bytes);
        firmware_calls++;
    }
    else if (!strcmp(file, asset_sha))
    {
        client->bytes = asset_bytes;
        client->size = sizeof(asset_bytes) - 1;
        client->chunked = true;
        asset_calls++;
    }
    else
        assert(false);
    return ESP_OK;
}
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client)
{
    if (client->status == 302)
    {
        esp_http_client_event_t header = {
            .event_id = HTTP_EVENT_ON_HEADER,
            .user_data = client->config.user_data,
            .header_key = "Location",
            .header_value = external_redirect ? "https://evil.example/update.json"
                                              : RELEASE_BASE "download/v0.1.0/update.json"};
        assert(client->config.event_handler(&header) == ESP_OK);
    }
    return client->chunked ? 0 : client->size;
}
int esp_http_client_get_status_code(esp_http_client_handle_t client)
{
    return client->status;
}
bool esp_http_client_is_chunked_response(esp_http_client_handle_t client)
{
    return client->chunked;
}
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t client)
{
    return client->offset == client->size;
}
esp_err_t esp_http_client_set_url(esp_http_client_handle_t client, const char *url)
{
    snprintf(client->url, sizeof(client->url), "%s", url);
    return ESP_OK;
}
int esp_http_client_read(esp_http_client_handle_t client, char *buffer, int capacity)
{
    size_t count = client->size - client->offset;
    if (body_cut && client->offset > 7)
        return 0;
    if (count > 7)
        count = 7;
    if (count > (size_t)capacity)
        count = capacity;
    memcpy(buffer, client->bytes + client->offset, count);
    if (asset_corrupt && client->bytes == asset_bytes && count)
        buffer[0] ^= 1;
    client->offset += count;
    return count;
}
esp_err_t esp_http_client_close(esp_http_client_handle_t client)
{
    (void)client;
    return ESP_OK;
}
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client)
{
    cleaned++;
    free(client);
    return ESP_OK;
}
esp_err_t app_ota_upload_verified(size_t size,
                                  app_ota_read_fn reader,
                                  void *context,
                                  bool (*verify)(void *context))
{
    assert(size == sizeof(firmware_bytes));
    /* A valódi flash-modult külön teszt vizsgálja. Itt a külső ellenőrzés időrendje számít. */
    assert(app_assets_ready(app_assets_identity()));
    char bytes[19];
    size_t used = 0;
    while (used < size)
    {
        int count =
            reader(context, bytes, size - used < sizeof(bytes) ? size - used : sizeof(bytes));
        if (count <= 0)
            return ESP_FAIL;
        used += count;
    }
    if (!verify(context))
        return ESP_ERR_INVALID_ARG;
    boot_targets++;
    return ESP_OK;
}
static void fixtures(void)
{
    hash_bytes(asset_bytes, sizeof(asset_bytes) - 1, asset_sha);
    cJSON *manifest = cJSON_Parse("{\"format\":\"zolee-assets\",\"version\":\"0.1.0\",\"assetAbi\":"
                                  "1,\"files\":[{\"path\":\"light/pump.png\"}]}");
    cJSON *entry = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(manifest, "files"), 0);
    cJSON_AddStringToObject(entry, "sha256", asset_sha);
    cJSON_AddNumberToObject(entry, "size", sizeof(asset_bytes) - 1);
    free(manifest_bytes);
    manifest_bytes = cJSON_PrintUnformatted(manifest);
    cJSON_Delete(manifest);
    memset(firmware_bytes, 0x5a, sizeof(firmware_bytes));
    memcpy(firmware_bytes + 288, app_assets_identity(), APP_ASSET_ID_SIZE);
    char firmware_sha[65], manifest_sha[65];
    hash_bytes(firmware_bytes, sizeof(firmware_bytes), firmware_sha);
    hash_bytes(manifest_bytes, strlen(manifest_bytes), manifest_sha);
    cJSON *descriptor = cJSON_Parse(
        "{\"format\":\"zolee-update\",\"version\":\"0.1.0\",\"manifest\":{},\"firmware\":{}}");
    cJSON_AddStringToObject(descriptor, "identity", app_assets_identity());
    entry = cJSON_GetObjectItemCaseSensitive(descriptor, "manifest");
    cJSON_AddStringToObject(entry, "sha256", manifest_sha);
    cJSON_AddNumberToObject(entry, "size", strlen(manifest_bytes));
    entry = cJSON_GetObjectItemCaseSensitive(descriptor, "firmware");
    cJSON_AddStringToObject(entry, "sha256", firmware_sha);
    cJSON_AddNumberToObject(entry, "size", sizeof(firmware_bytes));
    assert(app_release_descriptor_valid(descriptor));
    free(descriptor_bytes);
    descriptor_bytes = cJSON_PrintUnformatted(descriptor);
    cJSON_Delete(descriptor);
    external_redirect = asset_corrupt = body_cut = restarted = false;
}
static void completed(const char *phase_name, bool success)
{
    cJSON *status = app_release_status();
    assert(!strcmp(sm_json_string(status, "phase"), phase_name));
    assert(cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(status, "success")) == success);
    assert(opens == cleaned);
    cJSON_Delete(status);
}
int main(void)
{
    assert(app_release_init() == ESP_OK);
    assert(!app_release_url_allowed("https://github.com.evil.example/x"));
    assert(!app_release_url_allowed("http://github.com/x"));
    fixtures();
    external_redirect = true;
    assert(app_release_start(true) == ESP_OK);
    completed("release.unavailable", false);
    assert(!boot_targets);
    fixtures();
    body_cut = true;
    assert(app_release_start(true) == ESP_OK);
    completed("release.unavailable", false);
    assert(!boot_targets);
    fixtures();
    asset_corrupt = true;
    assert(app_release_start(true) == ESP_OK);
    completed("graphics.failed", false);
    assert(!boot_targets && !firmware_calls);
    fixtures();
    firmware_bytes[500] ^= 1;
    assert(app_release_start(true) == ESP_OK);
    completed("firmware.failed", false);
    assert(!boot_targets && !restarted);
    unsigned reused = asset_calls;
    fixtures();
    firmware_bytes[288 + 76] ^= 1; /* Ép teljes image hash mellett is kell az azonosítóegyezés. */
    cJSON *descriptor = sm_json_parse(descriptor_bytes, strlen(descriptor_bytes));
    char sha[65];
    hash_bytes(firmware_bytes, sizeof(firmware_bytes), sha);
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(
                             cJSON_GetObjectItemCaseSensitive(descriptor, "firmware"), "sha256"),
                         sha);
    free(descriptor_bytes);
    descriptor_bytes = cJSON_PrintUnformatted(descriptor);
    cJSON_Delete(descriptor);
    assert(app_release_start(true) == ESP_OK);
    completed("firmware.failed", false);
    assert(!boot_targets && asset_calls == reused);
    fixtures();
    task_deferred = true;
    assert(app_release_start(true) == ESP_OK && app_release_start(true) == ESP_ERR_INVALID_STATE);
    deferred_task(deferred_argument);
    task_deferred = false;
    completed("restarting", true);
    assert(boot_targets == 1 && restarted && asset_calls == reused);
    assert(app_release_start(true) == ESP_ERR_INVALID_STATE);
    /* Új bootot helyettesítünk; a tartós készlet már nem igényel hálózati blobot. */
    s_busy = false;
    fixtures();
    assert(app_release_start(false) == ESP_OK);
    completed("complete", true);
    assert(asset_calls == reused && boot_targets == 1);
    fixtures();
    snprintf(current_version, sizeof(current_version), "0.1.0");
    unsigned previous_firmware_calls = firmware_calls;
    assert(app_release_start(true) == ESP_OK);
    completed("complete", true);
    assert(!restarted && firmware_calls == previous_firmware_calls && boot_targets == 1);
    /* Azonos verziófelirat mellett eltérő grafikai azonosító is firmware-t igényel. */
    fixtures();
    char future_identity[APP_ASSET_ID_SIZE];
    snprintf(future_identity, sizeof(future_identity), "%s", app_assets_identity());
    future_identity[76] = future_identity[76] == 'a' ? 'b' : 'a';
    descriptor = sm_json_parse(descriptor_bytes, strlen(descriptor_bytes));
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(descriptor, "identity"), future_identity);
    memcpy(firmware_bytes + 288, future_identity, sizeof(future_identity));
    hash_bytes(firmware_bytes, sizeof(firmware_bytes), sha);
    cJSON_SetValuestring(cJSON_GetObjectItemCaseSensitive(
                             cJSON_GetObjectItemCaseSensitive(descriptor, "firmware"), "sha256"),
                         sha);
    free(descriptor_bytes);
    descriptor_bytes = cJSON_PrintUnformatted(descriptor);
    cJSON_Delete(descriptor);
    assert(app_release_start(false) == ESP_OK);
    completed("firmware.required", false);
    assert(firmware_calls == previous_firmware_calls);
    assert(app_release_start(true) == ESP_OK);
    completed("restarting", true);
    assert(firmware_calls == previous_firmware_calls + 1 && boot_targets == 2 && restarted);
    s_busy = false;
    clock_value = 0;
    assert(app_release_start(true) == ESP_OK);
    completed("clock.unavailable", false);
    assert(clock_waits == 30);
    clock_value = 1735689600;
    fixtures();
    network_up = false;
    assert(app_release_start(true) == ESP_OK);
    completed("wifi.unavailable", false);
    network_up = true;
    free(descriptor_bytes);
    free(manifest_bytes);
    puts("Native release: trusted redirects, chunked streams, metadata/blob/firmware hashes, "
         "identity, retry/dedup, busy and restart ordering OK");
}
