#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
typedef struct fake_http *esp_http_client_handle_t;
typedef struct
{
    int event_id;
    void *user_data;
    char *header_key, *header_value;
} esp_http_client_event_t;
#define HTTP_EVENT_ON_HEADER 1
typedef struct
{
    const char *url;
    int timeout_ms;
    esp_err_t (*crt_bundle_attach)(void *config);
    bool disable_auto_redirect;
    int buffer_size, buffer_size_tx;
    esp_err_t (*event_handler)(esp_http_client_event_t *event);
    void *user_data;
} esp_http_client_config_t;
esp_http_client_handle_t esp_http_client_init(const esp_http_client_config_t *config);
esp_err_t esp_http_client_open(esp_http_client_handle_t client, int size);
int64_t esp_http_client_fetch_headers(esp_http_client_handle_t client);
int esp_http_client_get_status_code(esp_http_client_handle_t client);
bool esp_http_client_is_chunked_response(esp_http_client_handle_t client);
bool esp_http_client_is_complete_data_received(esp_http_client_handle_t client);
esp_err_t esp_http_client_set_url(esp_http_client_handle_t client, const char *url);
int esp_http_client_read(esp_http_client_handle_t client, char *buffer, int capacity);
esp_err_t esp_http_client_close(esp_http_client_handle_t client);
esp_err_t esp_http_client_cleanup(esp_http_client_handle_t client);
