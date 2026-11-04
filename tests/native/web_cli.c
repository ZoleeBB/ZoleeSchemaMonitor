#include "app_diagnostics.h"
#include "app_network.h"
#include "app_store.h"
#include "app_web.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static cJSON *s_input_headers, *s_response_headers;
static const char *s_body;
static size_t s_offset;
static unsigned char *s_output;
static size_t s_size;
static const char *s_status = "200 OK";
static bool s_restart;
static esp_err_t (*s_handlers[2])(httpd_req_t *);
void *httpd_uri_match_wildcard;

void app_network_status(app_network_status_t *status)
{
    memset(status, 0, sizeof(*status));
    status->connected = true;
    snprintf(status->ip, sizeof(status->ip), "192.168.2.123");
}
void app_network_restart_later(void)
{
    s_restart = true;
}
void app_diagnostics_capture(app_diagnostics_t *data)
{
    memset(data, 0, sizeof(*data));
}
const esp_app_desc_t *esp_app_get_description(void)
{
    static const esp_app_desc_t app = {"native-test"};
    return &app;
}
esp_err_t httpd_start(httpd_handle_t *handle, const httpd_config_t *config)
{
    (void)config;
    *handle = (void *)1;
    return ESP_OK;
}
esp_err_t httpd_stop(httpd_handle_t handle)
{
    (void)handle;
    return ESP_OK;
}
esp_err_t httpd_register_uri_handler(httpd_handle_t handle, const httpd_uri_t *uri)
{
    (void)handle;
    s_handlers[uri->method] = uri->handler;
    return ESP_OK;
}
esp_err_t httpd_resp_set_status(httpd_req_t *request, const char *status)
{
    (void)request;
    s_status = status;
    return ESP_OK;
}
esp_err_t httpd_resp_set_hdr(httpd_req_t *request, const char *key, const char *value)
{
    (void)request;
    cJSON_AddStringToObject(s_response_headers, key, value);
    return ESP_OK;
}
esp_err_t httpd_resp_set_type(httpd_req_t *request, const char *type)
{
    return httpd_resp_set_hdr(request, "Content-Type", type);
}
esp_err_t httpd_resp_send_chunk(httpd_req_t *request, const char *data, int length)
{
    (void)request;
    if (length)
    {
        s_output = realloc(s_output, s_size + length);
        assert(s_output);
        memcpy(s_output + s_size, data, length);
        s_size += length;
    }
    return ESP_OK;
}
esp_err_t httpd_resp_send(httpd_req_t *request, const char *data, int length)
{
    return httpd_resp_send_chunk(request, data, length < 0 ? (int)strlen(data) : length);
}
size_t httpd_req_get_hdr_value_len(httpd_req_t *request, const char *key)
{
    (void)request;
    return strlen(sm_json_string(s_input_headers, key));
}
esp_err_t
httpd_req_get_hdr_value_str(httpd_req_t *request, const char *key, char *out, size_t length)
{
    (void)request;
    snprintf(out, length, "%s", sm_json_string(s_input_headers, key));
    return ESP_OK;
}
int httpd_req_recv(httpd_req_t *request, char *out, size_t size)
{
    size_t left = request->content_len - s_offset;
    if (size > left)
        size = left;
    memcpy(out, s_body + s_offset, size);
    s_offset += size;
    return (int)size;
}

int main(void)
{
    char *input = malloc(SM_PROJECT_BYTES + 4096);
    assert(input);
    size_t length = fread(input, 1, SM_PROJECT_BYTES + 4096, stdin);
    cJSON *json = sm_json_parse(input, length);
    free(input);
    assert(json);
    s_input_headers = cJSON_GetObjectItemCaseSensitive(json, "headers");
    s_body = sm_json_string(json, "body");
    httpd_req_t request = {.uri = sm_json_string(json, "path"),
                           .method = !strcmp(sm_json_string(json, "method"), "POST") ? HTTP_POST
                                                                                     : HTTP_GET,
                           .content_len = strlen(s_body)};
    s_response_headers = cJSON_CreateObject();
    (void)app_store_init();
    assert(app_web_start() == ESP_OK);
    esp_err_t result = s_handlers[request.method](&request);
    cJSON *response = cJSON_CreateObject();
    cJSON_AddNumberToObject(response, "result", result);
    cJSON_AddStringToObject(response, "status", s_status);
    cJSON_AddBoolToObject(response, "restart", s_restart);
    cJSON_AddItemToObject(response, "headers", s_response_headers);
    char *hex = malloc(s_size * 2 + 1);
    assert(hex);
    for (size_t i = 0; i < s_size; i++)
        snprintf(hex + i * 2, 3, "%02x", s_output[i]);
    hex[s_size * 2] = 0;
    cJSON_AddStringToObject(response, "bodyHex", hex);
    free(hex);
    free(s_output);
    char *text = cJSON_PrintUnformatted(response);
    assert(text);
    puts(text);
    free(text);
    cJSON_Delete(response);
    cJSON_Delete(json);
}

bool app_loxone_connected(void)
{
    return false;
}
cJSON *app_loxone_catalog(void)
{
    return cJSON_CreateArray();
}
cJSON *app_loxone_live(void)
{
    return cJSON_Parse("{\"connected\":false,\"values\":{},\"status\":\"test\"}");
}
