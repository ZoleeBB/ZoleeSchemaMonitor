#include "app_web.h"
#include "app_diagnostics.h"
#include "app_network.h"
#include "app_store.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct
{
    const char *path, *mime;
    const unsigned char *data;
    size_t size;
} web_file_t;
#include "web_bundle.inc"

static esp_err_t answer(httpd_req_t *request, const char *status, cJSON *json)
{
    httpd_resp_set_status(request, status);
    httpd_resp_set_type(request, "application/json; charset=utf-8");
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
    char *text = json ? cJSON_PrintUnformatted(json) : NULL;
    cJSON_Delete(json);
    esp_err_t result =
        httpd_resp_send(request, text ? text : "{\"error\":\"memory\"}", HTTPD_RESP_USE_STRLEN);
    free(text);
    return result;
}

static esp_err_t failure(httpd_req_t *request, const char *status, const char *message)
{
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "error", message);
    return answer(request, status, json);
}

static bool header(httpd_req_t *request, const char *key, char *buffer, size_t size)
{
    size_t length = httpd_req_get_hdr_value_len(request, key);
    return length > 0 && length < size &&
           httpd_req_get_hdr_value_str(request, key, buffer, size) == ESP_OK;
}

/* A kezelőfelület helyi címekre korlátozott: külső DNS-név nem használható
 * átjáróként. A módosítás saját originről és egyedi fejléc mellett fogadható. */
static bool permitted_host(httpd_req_t *request, char host[96])
{
    if (!header(request, "Host", host, 96))
        return false;
    char name[96];
    snprintf(name, sizeof(name), "%s", host);
    char *port = strchr(name, ':');
    if (port)
    {
        if (strcmp(port, ":80"))
            return false;
        *port = 0;
    }
    app_network_status_t network;
    app_network_status(&network);
    return !strcmp(name, "zolee-schema-monitor.local") || !strcmp(name, "zolee-schema-monitor") ||
           (network.connected && !strcmp(name, network.ip)) ||
           (network.recovery && !strcmp(name, "192.168.4.1"));
}

static bool permitted_write(httpd_req_t *request, const char *host)
{
    char marker[8], origin[128], expected[128], type[96];
    if (!header(request, "X-Zolee-Schema", marker, sizeof(marker)) || strcmp(marker, "1"))
        return false;
    if (httpd_req_get_hdr_value_len(request, "Origin"))
    {
        snprintf(expected, sizeof(expected), "http://%s", host);
        if (!header(request, "Origin", origin, sizeof(origin)) || strcmp(origin, expected))
            return false;
    }
    return header(request, "Content-Type", type, sizeof(type)) &&
           (!strcmp(type, "application/json") || !strcmp(type, "application/json; charset=utf-8"));
}

static cJSON *body(httpd_req_t *request)
{
    size_t length = request->content_len;
    if (!length || length > SM_PROJECT_BYTES + 512)
        return NULL;
    char *data = malloc(length);
    if (!data)
        return NULL;
    size_t used = 0;
    while (used < length)
    {
        int received = httpd_req_recv(request, data + used, length - used);
        if (received <= 0)
        {
            free(data);
            return NULL;
        }
        used += received;
    }
    cJSON *json = sm_json_parse(data, length);
    free(data);
    return json;
}

static cJSON *device(void)
{
    app_network_status_t network;
    app_network_status(&network);
    app_diagnostics_t metrics;
    app_diagnostics_capture(&metrics);
    cJSON *json = cJSON_CreateObject(), *wifi = cJSON_CreateObject(),
          *capabilities = cJSON_CreateObject();
    cJSON_AddBoolToObject(json, "development", false);
    cJSON_AddStringToObject(json, "firmware", esp_app_get_description()->version);
    cJSON_AddStringToObject(json,
                            "ip",
                            network.connected  ? network.ip
                            : network.recovery ? "192.168.4.1"
                                               : "");
    cJSON_AddBoolToObject(json, "connected", false);
    cJSON_AddNumberToObject(json, "uptime", metrics.uptime_seconds);
    cJSON_AddBoolToObject(wifi, "connected", network.connected);
    cJSON_AddNumberToObject(wifi, "rssi", network.rssi);
    cJSON_AddItemToObject(json, "wifi", wifi);
    cJSON_AddBoolToObject(capabilities, "ota", false);
    cJSON_AddBoolToObject(capabilities, "loxone", false);
    cJSON_AddBoolToObject(capabilities, "nativeRenderer", true);
    cJSON_AddItemToObject(json, "capabilities", capabilities);
    cJSON *integration = cJSON_CreateObject();
    cJSON_AddStringToObject(integration,
                            "hu",
                            "MS1: a helyi szerkesztő és a mentés bekötve; a natív megjelenítő, "
                            "Loxone és OTA még fejlesztés alatt.");
    cJSON_AddStringToObject(integration,
                            "en",
                            "MS1: local editor and storage integrated; native renderer, Loxone and "
                            "OTA are under development.");
    cJSON_AddItemToObject(json, "integrationStatus", integration);
    cJSON_AddItemToObject(json, "events", cJSON_CreateArray());
    return json;
}

static esp_err_t
file_response(httpd_req_t *request, const char *path, const char *mime, size_t maximum)
{
    struct stat info;
    if (stat(path, &info) != 0 || !S_ISREG(info.st_mode) || info.st_size <= 0 ||
        (uint64_t)info.st_size > maximum)
        return failure(request, "404 Not Found", "graphics.missing");
    FILE *file = fopen(path, "rb");
    if (!file)
        return failure(request, "503 Service Unavailable", "storage.read");
    char *buffer = malloc(4096);
    if (!buffer)
    {
        fclose(file);
        return failure(request, "503 Service Unavailable", "memory");
    }
    httpd_resp_set_type(request, mime);
    httpd_resp_set_hdr(request, "Cache-Control", "no-store");
    httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
    esp_err_t result = ESP_OK;
    size_t remaining = info.st_size;
    while (remaining && result == ESP_OK)
    {
        size_t size = fread(buffer, 1, remaining > 4096 ? 4096 : remaining, file);
        if (!size)
        {
            result = ESP_FAIL;
            break;
        }
        remaining -= size;
        result = httpd_resp_send_chunk(request, buffer, size);
    }
    if (result == ESP_OK)
        result = httpd_resp_send_chunk(request, NULL, 0);
    free(buffer);
    fclose(file);
    return result;
}

static esp_err_t get(httpd_req_t *request)
{
    char host[96];
    if (!permitted_host(request, host))
        return failure(request, "403 Forbidden", "host");
    const char *path = request->uri;
    if (!strcmp(path, "/api/device"))
        return answer(request, "200 OK", device());
    if (!strcmp(path, "/api/settings"))
        return answer(request, "200 OK", app_store_settings(true));
    if (!strcmp(path, "/api/project") || !strcmp(path, "/api/project/export"))
    {
        uint32_t revision;
        cJSON *project = app_store_project(&revision);
        if (!strcmp(path, "/api/project/export"))
        {
            httpd_resp_set_hdr(
                request, "Content-Disposition", "attachment; filename=ZoleeSchemaMonitor.json");
            return answer(request, "200 OK", project);
        }
        cJSON *json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "project", project);
        cJSON_AddNumberToObject(json, "revision", revision);
        return answer(request, "200 OK", json);
    }
    if (!strcmp(path, "/api/loxone/catalog"))
    {
        cJSON *json = cJSON_CreateObject();
        cJSON_AddItemToObject(json, "catalog", cJSON_CreateArray());
        return answer(request, "200 OK", json);
    }
    if (!strcmp(path, "/api/live"))
    {
        cJSON *json = cJSON_CreateObject();
        cJSON_AddBoolToObject(json, "connected", false);
        cJSON_AddItemToObject(json, "values", cJSON_CreateObject());
        cJSON_AddStringToObject(
            json, "status", "Loxone integráció fejlesztés alatt / Loxone integration pending");
        return answer(request, "200 OK", json);
    }
    if (!strcmp(path, "/api/diagnostics"))
    {
        app_diagnostics_t data;
        app_diagnostics_capture(&data);
        app_network_status_t network;
        app_network_status(&network);
        cJSON *json = cJSON_CreateObject();
        cJSON_AddNumberToObject(json, "uptime", data.uptime_seconds);
        cJSON_AddNumberToObject(json, "cpu0", data.cpu[0]);
        cJSON_AddNumberToObject(json, "cpu1", data.cpu[1]);
        cJSON_AddNumberToObject(json, "internalFree", data.internal_free);
        cJSON_AddNumberToObject(json, "internalMinimum", data.internal_minimum);
        cJSON_AddNumberToObject(json, "psramFree", data.psram_free);
        cJSON_AddNumberToObject(json, "dmaLargest", data.dma_largest);
        cJSON_AddBoolToObject(json, "sdMounted", app_store_ready());
        cJSON_AddBoolToObject(json, "storageRecovery", app_store_recovered());
        cJSON_AddBoolToObject(json, "recoveryWifi", network.recovery);
        cJSON_AddItemToObject(json, "tasks", cJSON_CreateArray());
        return answer(request, "200 OK", json);
    }
    if (!strcmp(path, "/api/graphics"))
    {
        const char *manifest = "/sdcard/graphics/manifest.json";
        struct stat info;
        if (!stat(manifest, &info))
            return file_response(request, manifest, "application/json", 262144);
        cJSON *json = cJSON_CreateObject();
        cJSON_AddStringToObject(json, "version", "not-installed");
        cJSON_AddNumberToObject(json, "assetAbi", 1);
        cJSON_AddItemToObject(json, "files", cJSON_CreateArray());
        return answer(request, "200 OK", json);
    }
    if (!strncmp(path, "/graphics/", 10))
    {
        const char *name = path + 10;
        if (strncmp(name, "light/", 6) && strncmp(name, "dark/", 5))
            return failure(request, "404 Not Found", "graphics.path");
        const char *id = strchr(name, '/') + 1;
        size_t length = strlen(id);
        char asset[65];
        if (length < 5 || length - 4 >= sizeof(asset) || strcmp(id + length - 4, ".svg"))
            return failure(request, "404 Not Found", "graphics.path");
        memcpy(asset, id, length - 4);
        asset[length - 4] = 0;
        if (!sm_asset_find(asset))
            return failure(request, "404 Not Found", "graphics.asset");
        char filename[160];
        snprintf(filename, sizeof(filename), "/sdcard/graphics/%s", name);
        return file_response(request, filename, "image/svg+xml", 262144);
    }
    const char *routes[] = {
        "/", "/editor", "/settings", "/loxone", "/graphics", "/backup", "/update", "/diagnostics"};
    for (unsigned i = 0; i < sizeof(routes) / sizeof(routes[0]); i++)
        if (!strcmp(path, routes[i]))
        {
            path = "/index.html";
            break;
        }
    for (unsigned i = 0; i < sizeof(s_web_files) / sizeof(s_web_files[0]); i++)
    {
        const web_file_t *file = &s_web_files[i];
        if (strcmp(path, file->path))
            continue;
        httpd_resp_set_type(request, file->mime);
        httpd_resp_set_hdr(request, "Content-Encoding", "gzip");
        httpd_resp_set_hdr(request, "Cache-Control", "no-cache");
        httpd_resp_set_hdr(request, "X-Content-Type-Options", "nosniff");
        httpd_resp_set_hdr(
            request,
            "Content-Security-Policy",
            "default-src 'self'; img-src 'self' blob: data:; style-src 'self' 'unsafe-inline'; "
            "script-src 'self'; connect-src 'self'; frame-ancestors 'none'; base-uri 'self'");
        return httpd_resp_send(request, (const char *)file->data, file->size);
    }
    return failure(request, "404 Not Found", "route");
}

static esp_err_t post(httpd_req_t *request)
{
    char host[96];
    if (!permitted_host(request, host) || !permitted_write(request, host))
        return failure(request, "403 Forbidden", "origin/header");
    if (!strcmp(request->uri, "/api/update") || !strcmp(request->uri, "/api/graphics/refresh"))
        return failure(
            request,
            "501 Not Implemented",
            "Device OTA integration pending / Készülékes OTA integráció fejlesztés alatt");
    if (request->content_len > SM_PROJECT_BYTES + 512)
        return failure(request, "413 Content Too Large", "body.size");
    cJSON *json = body(request);
    if (!json)
        return failure(request, "400 Bad Request", "json");
    char error[SM_ERROR_SIZE] = "request.fields";
    esp_err_t result = ESP_ERR_INVALID_ARG;
    cJSON *response = NULL;
    bool restart = false;
    if (!strcmp(request->uri, "/api/project"))
    {
        const cJSON *project = cJSON_GetObjectItemCaseSensitive(json, "project");
        double expected = sm_json_number(json, "revision", -1);
        uint32_t revision;
        if (expected >= 0 && expected <= UINT32_MAX && expected == floor(expected))
        {
            result = app_store_save_project(project, (uint32_t)expected, &revision, error);
            if (result == ESP_OK)
            {
                response = cJSON_CreateObject();
                cJSON_AddNumberToObject(response, "revision", revision);
            }
        }
    }
    else if (!strcmp(request->uri, "/api/settings") || !strcmp(request->uri, "/api/loxone"))
    {
        result = app_store_save_settings(json, !strcmp(request->uri, "/api/loxone"), error);
        if (result == ESP_OK)
        {
            response = app_store_settings(true);
            restart = true;
        }
    }
    else
        snprintf(error, sizeof(error), "route");
    cJSON_Delete(json);
    if (result != ESP_OK)
    {
        const char *status = result == ESP_ERR_INVALID_STATE ? "409 Conflict"
                           : result == ESP_FAIL              ? "503 Service Unavailable"
                                                             : "400 Bad Request";
        return failure(request, status, error);
    }
    esp_err_t sent = answer(request, "200 OK", response);
    if (restart)
        app_network_restart_later();
    return sent;
}

esp_err_t app_web_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 12288;
    config.max_open_sockets = 5;
    config.lru_purge_enable = true;
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;
    config.uri_match_fn = httpd_uri_match_wildcard;
    httpd_handle_t server;
    esp_err_t result = httpd_start(&server, &config);
    if (result != ESP_OK)
        return result;
    httpd_uri_t read = {.uri = "/*", .method = HTTP_GET, .handler = get};
    httpd_uri_t write = {.uri = "/*", .method = HTTP_POST, .handler = post};
    result = httpd_register_uri_handler(server, &read);
    if (result == ESP_OK)
        result = httpd_register_uri_handler(server, &write);
    if (result != ESP_OK)
        httpd_stop(server);
    return result;
}
