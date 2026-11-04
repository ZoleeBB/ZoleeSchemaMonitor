#pragma once
#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>
#define HTTP_GET 0
#define HTTP_POST 1
#define HTTPD_RESP_USE_STRLEN -1

typedef struct
{
    const char *uri;
    int method;
    size_t content_len;
} httpd_req_t;
typedef void *httpd_handle_t;
typedef struct
{
    int stack_size, max_open_sockets;
    bool lru_purge_enable;
    int recv_wait_timeout, send_wait_timeout;
    void *uri_match_fn;
} httpd_config_t;
#define HTTPD_DEFAULT_CONFIG() ((httpd_config_t){0})
typedef struct
{
    const char *uri;
    int method;
    esp_err_t (*handler)(httpd_req_t *);
} httpd_uri_t;
extern void *httpd_uri_match_wildcard;
esp_err_t httpd_start(httpd_handle_t *, const httpd_config_t *);
esp_err_t httpd_stop(httpd_handle_t);
esp_err_t httpd_register_uri_handler(httpd_handle_t, const httpd_uri_t *);
esp_err_t httpd_resp_set_status(httpd_req_t *, const char *);
esp_err_t httpd_resp_set_type(httpd_req_t *, const char *);
esp_err_t httpd_resp_set_hdr(httpd_req_t *, const char *, const char *);
esp_err_t httpd_resp_send(httpd_req_t *, const char *, int);
esp_err_t httpd_resp_send_chunk(httpd_req_t *, const char *, int);
size_t httpd_req_get_hdr_value_len(httpd_req_t *, const char *);
esp_err_t httpd_req_get_hdr_value_str(httpd_req_t *, const char *, char *, size_t);
int httpd_req_recv(httpd_req_t *, char *, size_t);
