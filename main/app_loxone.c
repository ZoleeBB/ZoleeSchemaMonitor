/* A testvérprojekt 2dabb18 commitjának olvasási transzportja és hitelesítése.
 * Minden hálózati művelet egyetlen workerből fut; nincsenek vezérlőparancsok. */
#include "app_loxone.h"
#include "app_store.h"
#include "app_network.h"
#include "schema_lx_cache.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "freertos/idf_additions.h"
#include "esp_websocket_client.h"
#include "esp_http_client.h"
#include "esp_random.h"
#include "esp_mac.h"
#include "esp_timer.h"
#include "esp_netif_sntp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "mbedtls/aes.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include "mbedtls/pk.h"
#include "mbedtls/platform_util.h"
#include <math.h>
#include <stdatomic.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define LX_TIMEOUT_MS 12000
#define LX_QUEUE_DEPTH 8
#define LX_STRUCTURE_MAX SM_LX_STRUCTURE_BYTES
#define LX_MAX_RX (LX_STRUCTURE_MAX + 1024U)
typedef struct
{
    char host[128];
    unsigned short port;
    bool tls;
    char ca_pem[4096], username[96], password[128];
} lx_config_t;
typedef struct
{
    int type;
    size_t length;
    unsigned char *data;
} message_t;
static SemaphoreHandle_t s_lock;
static QueueHandle_t s_messages;
static cJSON *s_catalog;
static char s_status[192] = "Nincs Loxone kapcsolat / Loxone disconnected";
static char s_stage[80], s_failure[192];
static esp_websocket_client_handle_t s_ws;
static unsigned char *s_rx;
static size_t s_rx_len, s_rx_cap;
static bool s_rx_bad, s_live;
static sm_lx_frame_t s_frame;
static atomic_bool s_dead;
static atomic_int s_rx_failure;
enum
{
    LX_RX_OK,
    LX_RX_TOO_LARGE,
    LX_RX_NO_MEMORY,
    LX_RX_BAD_HEADER,
    LX_RX_QUEUE_FULL
};
static unsigned char s_aes_key[32], s_iv[16];
static char s_salt[17];
static char *s_public_key;
static const lx_config_t *s_session_config;
static int64_t s_last_response, s_token_refresh_us;
static EXT_RAM_BSS_ATTR sm_lx_cache_t s_cache;
static bool update_values(int type, const uint8_t *data, size_t size)
{
    if (type != 2)
        return true;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool ok = sm_lx_cache_packet(&s_cache, data, size);
    if (!ok)
        s_live = false;
    xSemaphoreGive(s_lock);
    return ok;
}
static int64_t lx_token_refresh_delay_us(int64_t until, int64_t now)
{
    int64_t seconds = 3600;
    if (until > 0 && now >= 1577836800)
    {
        int64_t remaining = until + 1230768000LL - now;
        seconds = remaining > 2 ? remaining / 2 : 1;
        if (seconds > 3600)
            seconds = 3600;
    }
    return seconds * 1000000LL;
}
static void status(const char *message)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    snprintf(s_status, sizeof(s_status), "%s", message);
    xSemaphoreGive(s_lock);
}
static void phase(const char *label)
{
    snprintf(s_stage, sizeof(s_stage), "%s", label);
    s_failure[0] = 0;
    status(label);
}
static bool failed(const char *reason, int code)
{
    if (code)
        snprintf(s_failure, sizeof(s_failure), "%.70s: %.85s (%d)", s_stage, reason, code);
    else
        snprintf(s_failure, sizeof(s_failure), "%.70s: %.110s", s_stage, reason);
    return false;
}
static void report_failure(const char *fallback)
{
    status(s_failure[0] ? s_failure : fallback);
}
static const char *string(const cJSON *o, const char *k)
{
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(o, k);
    return cJSON_IsString(v) ? v->valuestring : "";
}
static void hex(const unsigned char *in, size_t n, char *out)
{
    static const char h[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++)
    {
        out[2 * i] = h[in[i] >> 4];
        out[2 * i + 1] = h[in[i] & 15];
    }
    out[2 * n] = 0;
}
static bool unhex(const char *in, unsigned char *out, size_t max, size_t *n)
{
    size_t len = strlen(in);
    if (!len || len % 2 || len / 2 > max)
        return false;
    for (size_t i = 0; i < len; i++)
        if (!strchr("0123456789abcdefABCDEF", in[i]))
            return false;
    for (size_t i = 0; i < len / 2; i++)
    {
        unsigned v;
        if (sscanf(in + 2 * i, "%2x", &v) != 1)
            return false;
        out[i] = v;
    }
    *n = len / 2;
    return true;
}
static char *urlencode(const char *in)
{
    size_t n = strlen(in);
    char *out = malloc(n * 3 + 1);
    if (!out)
        return NULL;
    char *p = out;
    for (size_t i = 0; i < n; i++)
    {
        unsigned char c = in[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
            strchr("-_.~", c))
            *p++ = c;
        else
        {
            snprintf(p, 4, "%%%02X", c);
            p += 3;
        }
    }
    *p = 0;
    return out;
}
static void free_message(message_t *m)
{
    free(m->data);
    m->data = NULL;
}
static bool queue_message(unsigned type, const unsigned char *data, size_t length, void *context)
{
    (void)data;
    (void)context;
    if (type == 5)
    {
        s_dead = true;
        return true;
    }
    message_t message = {.type = (int)type, .length = length};
    if (length)
    {
        message.data = s_rx;
        s_rx = NULL;
        s_rx_cap = 0;
    }
    if (xQueueSend(s_messages, &message, 0) != pdTRUE)
    {
        free_message(&message);
        s_rx_failure = LX_RX_QUEUE_FULL;
        return false;
    }
    return true;
}

static void receive_event(void *arg, esp_event_base_t base, int32_t event, void *data)
{
    (void)arg;
    (void)base;
    if (event == WEBSOCKET_EVENT_DISCONNECTED || event == WEBSOCKET_EVENT_ERROR)
    {
        s_dead = true;
        return;
    }
    if (event != WEBSOCKET_EVENT_DATA || s_dead)
        return;
    esp_websocket_event_data_t *e = data;
    if (e->op_code == 8)
    {
        s_dead = true;
        return;
    }
    if (e->op_code != 0 && e->op_code != 1 && e->op_code != 2)
        return;
    if (e->data_len < 0 || e->payload_offset < 0 || e->payload_len < 0)
    {
        s_dead = true;
        return;
    }
    if (e->payload_offset == 0 && e->op_code != 0)
    {
        s_rx_len = 0;
        s_rx_bad = false;
    }
    if ((size_t)e->data_len > LX_MAX_RX - s_rx_len)
    {
        s_rx_failure = LX_RX_TOO_LARGE;
        s_rx_bad = true;
        s_dead = true;
        return;
    }
    size_t need = s_rx_len + e->data_len + 1;
    if (need > s_rx_cap)
    {
        size_t cap = s_rx_cap ? s_rx_cap : 4096;
        while (cap < need)
        {
            if (cap > (LX_MAX_RX + 1) / 2)
            {
                cap = LX_MAX_RX + 1;
                break;
            }
            cap *= 2;
        }
        unsigned char *next = realloc(s_rx, cap);
        if (!next)
        {
            s_rx_failure = LX_RX_NO_MEMORY;
            s_rx_bad = true;
            s_dead = true;
            return;
        }
        s_rx = next;
        s_rx_cap = cap;
    }
    memcpy(s_rx + s_rx_len, e->data_ptr, e->data_len);
    s_rx_len += e->data_len;
    s_rx[s_rx_len] = 0;
    if ((size_t)e->payload_offset + (size_t)e->data_len != (size_t)e->payload_len || !e->fin)
        return;
    if (s_rx_bad)
        return;
    if (!sm_lx_message(&s_frame, s_rx, s_rx_len, queue_message, NULL))
    {
        if (s_rx_failure == LX_RX_OK)
            s_rx_failure = LX_RX_BAD_HEADER;
        s_dead = true;
    }
    s_rx_len = 0;
}

static void mark_response(void)
{
    s_last_response = esp_timer_get_time();
}
static bool send_plain(const char *command)
{
    return !s_dead && esp_websocket_client_send_text(
                          s_ws, command, strlen(command), pdMS_TO_TICKS(LX_TIMEOUT_MS)) ==
                          (int)strlen(command);
}
static bool send_encrypted(const char *command)
{
    unsigned char salt[8], iv[16];
    char sh[17];
    esp_fill_random(salt, sizeof(salt));
    hex(salt, sizeof(salt), sh);
    size_t n = strlen(command) + strlen(sh) + strlen(s_salt) + 16, padded = (n + 15) & ~(size_t)15;
    unsigned char *input = calloc(1, padded + 1), *cipher = malloc(padded),
                  *base = malloc(padded * 2 + 8);
    if (!input || !cipher || !base)
    {
        free(input);
        free(cipher);
        free(base);
        return false;
    }
    int actual = s_salt[0]
                   ? snprintf((char *)input, padded + 1, "nextSalt/%s/%s/%s", s_salt, sh, command)
                   : snprintf((char *)input, padded + 1, "salt/%s/%s", sh, command);
    bool ok = actual > 0 && (size_t)actual <= padded;
    size_t b64 = 0;
    mbedtls_aes_context aes;
    mbedtls_aes_init(&aes);
    memcpy(iv, s_iv, sizeof(iv));
    if (ok)
        ok = mbedtls_aes_setkey_enc(&aes, s_aes_key, 256) == 0 &&
             mbedtls_aes_crypt_cbc(&aes, MBEDTLS_AES_ENCRYPT, padded, iv, input, cipher) == 0 &&
             mbedtls_base64_encode(base, padded * 2 + 8, &b64, cipher, padded) == 0;
    char *escaped = NULL, *wrapped = NULL;
    if (ok)
    {
        base[b64] = 0;
        escaped = urlencode((char *)base);
        if (escaped)
        {
            wrapped = malloc(strlen(escaped) + 16);
            if (wrapped)
                sprintf(wrapped, "jdev/sys/enc/%s", escaped);
        }
        ok = wrapped && send_plain(wrapped);
    }
    if (ok)
        snprintf(s_salt, sizeof(s_salt), "%s", sh);
    mbedtls_aes_free(&aes);
    if (input)
        mbedtls_platform_zeroize(input, padded);
    free(input);
    free(cipher);
    free(base);
    free(escaped);
    free(wrapped);
    return ok;
}
static cJSON *reply(const char *command, bool encrypt)
{
    if (!(encrypt ? send_encrypted(command) : send_plain(command)))
    {
        failed("A kérés nem küldhető el.", 0);
        return NULL;
    }
    int64_t deadline = esp_timer_get_time() + LX_TIMEOUT_MS * 1000LL;
    while (!s_dead && esp_timer_get_time() < deadline)
    {
        message_t m;
        if (xQueueReceive(s_messages, &m, pdMS_TO_TICKS(100)) != pdTRUE)
            continue;
        mark_response();
        if (m.type != 0)
        {
            if (s_session_config && !update_values(m.type, m.data, m.length))
                s_dead = true;
            free_message(&m);
            continue;
        }
        cJSON *root = cJSON_ParseWithLength((char *)m.data, m.length);
        free_message(&m);
        cJSON *ll = cJSON_GetObjectItemCaseSensitive(root, "LL");
        cJSON *code = cJSON_GetObjectItemCaseSensitive(ll, "Code");
        if (!code)
            code = cJSON_GetObjectItemCaseSensitive(ll, "code");
        bool ok = (cJSON_IsNumber(code) && code->valueint == 200) ||
                  (cJSON_IsString(code) && strcmp(code->valuestring, "200") == 0);
        if (!ok)
        {
            int response_code = cJSON_IsNumber(code) ? code->valueint
                              : cJSON_IsString(code) ? atoi(code->valuestring)
                                                     : 0;
            failed(response_code ? "A Miniserver elutasította a kérést."
                                 : "Érvénytelen Miniserver-válasz.",
                   response_code);
        }
        const cJSON *returned = cJSON_GetObjectItemCaseSensitive(ll, "value");
        /* A parancsnyugta 200 mellett érték nélkül is érvényes. A kapcsoló
         * sikerét ettől függetlenül kizárólag a bináris állapot igazolja. */
        cJSON *value =
            ok ? (returned ? cJSON_Duplicate(returned, true) : cJSON_CreateNull()) : NULL;
        cJSON_Delete(root);
        return value;
    }
    failed(s_dead ? "A kapcsolat megszakadt." : "12 másodpercen belül nem érkezett válasz.", 0);
    return NULL;
}
static int rng(void *ctx, unsigned char *out, size_t len)
{
    (void)ctx;
    esp_fill_random(out, len);
    return 0;
}
static bool key_exchange(void)
{
    phase("Loxone titkosítási kulcscsere");
    if (!s_public_key)
        return failed("Hiányzó nyilvános kulcs.", 0);
    /* Loxone egyes verziói SPKI kulcsot CERTIFICATE fejléccel küldenek. */
    const char *body = strstr(s_public_key, "-----BEGIN CERTIFICATE-----");
    char *pem = NULL;
    if (body)
    {
        body += 27;
        const char *end = strstr(body, "-----END CERTIFICATE-----");
        if (end)
        {
            size_t n = end - body;
            pem = malloc(n + 64);
            if (pem)
                snprintf(pem,
                         n + 64,
                         "-----BEGIN PUBLIC KEY-----\n%.*s\n-----END PUBLIC KEY-----\n",
                         (int)n,
                         body);
        }
    }
    else
        pem = strdup(s_public_key);
    if (!pem)
        return failed("A nyilvános kulcs formátuma nem értelmezhető.", 0);
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    int pk_result = mbedtls_pk_parse_public_key(&pk, (unsigned char *)pem, strlen(pem) + 1);
    bool ok = pk_result == 0;
    free(pem);
    if (!ok)
        failed("A nyilvános kulcs feldolgozása sikertelen.", pk_result);
    unsigned char encrypted[512], base[704];
    char key[65], iv[33], plain[100];
    size_t n = 0, b64 = 0;
    esp_fill_random(s_aes_key, sizeof(s_aes_key));
    esp_fill_random(s_iv, sizeof(s_iv));
    hex(s_aes_key, 32, key);
    hex(s_iv, 16, iv);
    snprintf(plain, sizeof(plain), "%s:%s", key, iv);
    if (ok)
    {
        int result = mbedtls_pk_encrypt(&pk,
                                        (unsigned char *)plain,
                                        strlen(plain),
                                        encrypted,
                                        &n,
                                        sizeof(encrypted),
                                        rng,
                                        NULL);
        ok = result == 0;
        if (!ok)
            failed("Az RSA kulcscsere előkészítése sikertelen.", result);
    }
    mbedtls_pk_free(&pk);
    mbedtls_platform_zeroize(plain, sizeof(plain));
    mbedtls_platform_zeroize(key, sizeof(key));
    if (ok)
        ok = mbedtls_base64_encode(base, sizeof(base) - 1, &b64, encrypted, n) == 0;
    if (!ok)
        return false;
    /* A kulcscsere nyers Base64-t vár; csak az AES parancsok URL-kódoltak. */
    base[b64] = 0;
    char command[sizeof(base) + 32];
    snprintf(command, sizeof(command), "jdev/sys/keyexchange/%s", base);
    cJSON *value = reply(command, false);
    ok = value != NULL;
    cJSON_Delete(value);
    return ok;
}
static bool hash_hmac(const cJSON *key, const char *input, char out[65])
{
    const char *algorithm = string(key, "hashAlg");
    mbedtls_md_type_t type = strcmp(algorithm, "SHA256") == 0 ? MBEDTLS_MD_SHA256
                           : strcmp(algorithm, "SHA1") == 0   ? MBEDTLS_MD_SHA1
                                                              : MBEDTLS_MD_NONE;
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(type);
    if (!md)
        return false;
    unsigned char k[64], digest[32];
    size_t n;
    if (!unhex(string(key, "key"), k, sizeof(k), &n))
        return false;
    bool ok = mbedtls_md_hmac(md, k, n, (unsigned char *)input, strlen(input), digest) == 0;
    if (ok)
        hex(digest, mbedtls_md_get_size(md), out);
    mbedtls_platform_zeroize(k, sizeof(k));
    return ok;
}
static void schedule_token_refresh(const cJSON *value)
{
    const cJSON *until = cJSON_GetObjectItemCaseSensitive(value, "validUntil");
    int64_t valid_until =
        cJSON_IsNumber(until) && until->valuedouble > 0 && until->valuedouble < 1e12
            ? (int64_t)until->valuedouble
            : 0;
    s_token_refresh_us =
        esp_timer_get_time() + lx_token_refresh_delay_us(valid_until, (int64_t)time(NULL));
}
static bool acquire_token(const lx_config_t *cfg, char *token, size_t token_size)
{
    phase("Loxone felhasználói hitelesítési kulcs");
    char *user = urlencode(cfg->username);
    if (!user)
        return false;
    char command[1536];
    snprintf(command, sizeof(command), "jdev/sys/getkey2/%s", user);
    cJSON *key = reply(command, false);
    if (!key)
    {
        free(user);
        return false;
    }
    const char *algorithm = string(key, "hashAlg");
    const mbedtls_md_info_t *md =
        mbedtls_md_info_from_type(strcmp(algorithm, "SHA256") == 0 ? MBEDTLS_MD_SHA256
                                  : strcmp(algorithm, "SHA1") == 0 ? MBEDTLS_MD_SHA1
                                                                   : MBEDTLS_MD_NONE);
    char salted[512], password_hash[65], input[256], hmac[65];
    unsigned char digest[32];
    int n = snprintf(salted, sizeof(salted), "%s:%s", cfg->password, string(key, "salt"));
    bool ok = md && n > 0 && n < (int)sizeof(salted) &&
              mbedtls_md(md, (unsigned char *)salted, n, digest) == 0;
    if (ok)
    {
        hex(digest, mbedtls_md_get_size(md), password_hash);
        for (char *p = password_hash; *p; p++)
            if (*p >= 'a' && *p <= 'f')
                *p -= 32;
        snprintf(input, sizeof(input), "%s:%s", cfg->username, password_hash);
        ok = hash_hmac(key, input, hmac);
    }
    cJSON_Delete(key);
    mbedtls_platform_zeroize(salted, sizeof(salted));
    mbedtls_platform_zeroize(password_hash, sizeof(password_hash));
    mbedtls_platform_zeroize(input, sizeof(input));
    if (ok)
    {
        unsigned char mac[6];
        if (esp_efuse_mac_get_default(mac) != ESP_OK)
        {
            free(user);
            return false;
        }
        snprintf(command,
                 sizeof(command),
                 "jdev/sys/getjwt/%s/%s/4/7f30a221-5e2d-4100-918a%02x%02x%02x%02x%02x%02x/"
                 "ZoleeSchemaMonitor",
                 hmac,
                 user,
                 mac[0],
                 mac[1],
                 mac[2],
                 mac[3],
                 mac[4],
                 mac[5]);
        phase("Loxone felhasználói bejelentkezés");
        cJSON *v = reply(command, true);
        const char *t = string(v, "token");
        ok = *t && strlen(t) < token_size;
        if (ok)
        {
            snprintf(token, token_size, "%s", t);
            schedule_token_refresh(v);
        }
        cJSON_Delete(v);
    }
    free(user);
    return ok;
}
static bool token_command(const lx_config_t *cfg, char *token, const char *operation)
{
    cJSON *key = reply("jdev/sys/getkey", false);
    char hmac[65];
    /* getkey régebbi firmware esetén egyszerű hex string, SHA1 alapú. */
    if (cJSON_IsString(key))
    {
        cJSON *o = cJSON_CreateObject();
        cJSON_AddStringToObject(o, "key", key->valuestring);
        cJSON_AddStringToObject(o, "hashAlg", "SHA1");
        cJSON_Delete(key);
        key = o;
    }
    bool ok = key && hash_hmac(key, token, hmac);
    cJSON_Delete(key);
    if (!ok)
        return false;
    char *user = urlencode(cfg->username);
    if (!user)
        return false;
    char cmd[768];
    snprintf(cmd,
             sizeof(cmd),
             "%s%s/%s/%s",
             strcmp(operation, "authwithtoken") == 0 ? "" : "jdev/sys/",
             operation,
             hmac,
             user);
    free(user);
    cJSON *v = reply(cmd, true);
    ok = v != NULL;
    if (ok && strcmp(operation, "refreshjwt") == 0)
    {
        const char *t = string(v, "token");
        ok = *t && strlen(t) < 1024;
        if (ok)
            snprintf(token, 1024, "%s", t);
    }
    if (ok && strcmp(operation, "killtoken") != 0)
        schedule_token_refresh(v);
    cJSON_Delete(v);
    return ok;
}
static bool renew_token(const lx_config_t *cfg, char token[1024])
{
    if (token_command(cfg, token, "refreshjwt"))
        return true;
    return !s_dead && acquire_token(cfg, token, 1024);
}
static bool download_catalog(int *response_code)
{
    *response_code = 0;
    phase("Loxone adatlista letöltése");
    /* LoxAPP3.json esetén enc támogatott, fenc (titkosított válasz) nem. */
    if (!send_encrypted("data/LoxAPP3.json"))
        return failed("Az adatlista kérése nem küldhető el.", 0);
    int64_t deadline = esp_timer_get_time() + LX_TIMEOUT_MS * 1000LL;
    while (!s_dead && esp_timer_get_time() < deadline)
    {
        message_t m;
        if (xQueueReceive(s_messages, &m, pdMS_TO_TICKS(100)) != pdTRUE)
            continue;
        if (m.type != 0)
        {
            free_message(&m);
            continue;
        }
        cJSON *catalog = sm_lx_catalog((char *)m.data, m.length);
        if (!catalog)
        {
            /* A hibaválasz nem katalógus: ne nyeljük el időtúllépésként. */
            cJSON *root = cJSON_ParseWithLength((char *)m.data, m.length);
            const cJSON *ll = cJSON_GetObjectItemCaseSensitive(root, "LL");
            const cJSON *code = cJSON_GetObjectItemCaseSensitive(ll, "Code");
            if (!code)
                code = cJSON_GetObjectItemCaseSensitive(ll, "code");
            int result = cJSON_IsNumber(code) ? code->valueint
                       : cJSON_IsString(code) ? atoi(code->valuestring)
                                              : 0;
            *response_code = result;
            if (ll)
                failed(result == 403 ? "Az adatlista hozzáférése tiltott. Ellenőrizd a "
                                       "felhasználói jogosultságot."
                                     : "A Miniserver adatlista helyett parancsválaszt küldött.",
                       result);
            else if (m.length > LX_STRUCTURE_MAX)
                failed("Az adatlista meghaladja az 512 KiB méretkorlátot.", 0);
            else if (!root)
                failed("A válasz nem érvényes JSON, hiányos, vagy nincs elég memória.", 0);
            else if (!cJSON_IsObject(cJSON_GetObjectItemCaseSensitive(root, "controls")))
                failed("A válaszból hiányzik a controls adatlista.", 0);
            else
                failed("Az állapotlista feldolgozása sikertelen (512 állapot, mélység- vagy "
                       "memóriakorlát).",
                       0);
            cJSON_Delete(root);
            free_message(&m);
            return false;
        }
        free_message(&m);
        xSemaphoreTake(s_lock, portMAX_DELAY);
        cJSON_Delete(s_catalog);
        s_catalog = catalog;
        xSemaphoreGive(s_lock);
        return true;
    }
    if (s_rx_failure == LX_RX_TOO_LARGE)
        return failed("A fogadott üzenet meghaladja az 513 KiB vételi korlátot.", 0);
    if (s_rx_failure == LX_RX_NO_MEMORY)
        return failed("Nincs elég memória az adatlista fogadásához.", 0);
    if (s_rx_failure == LX_RX_BAD_HEADER)
        return failed("Érvénytelen Loxone üzenetfejléc érkezett.", 0);
    if (s_rx_failure == LX_RX_QUEUE_FULL)
        return failed("A fogadott üzenetek sora megtelt.", 0);
    return failed(s_dead ? "A kapcsolat megszakadt az adatlista fogadása közben."
                         : "12 másodpercen belül nem érkezett adatlista.",
                  0);
}
/* A HTTP fájlútvonal egyes Miniszervereken elérhető akkor is, ha a WS 404-et ad.
 * Csak 404-re váltunk át: a jogosultsági hibákat nem kerüljük meg. */
static char *http_body(const lx_config_t *cfg, const char *path, size_t limit, size_t *length)
{
    size_t url_size = strlen(path) + strlen(cfg->host) + 32;
    char *url = malloc(url_size);
    if (!url)
    {
        failed("Nincs elég memória a HTTP kéréshez.", 0);
        return NULL;
    }
    snprintf(url,
             url_size,
             "%s://%s:%u/%s",
             cfg->tls ? "https" : "http",
             cfg->host,
             (unsigned)cfg->port,
             path);
    esp_http_client_config_t options = {.url = url,
                                        .timeout_ms = LX_TIMEOUT_MS,
                                        .cert_pem = cfg->tls ? cfg->ca_pem : NULL,
                                        .disable_auto_redirect = true};
    esp_http_client_handle_t http = esp_http_client_init(&options);
    if (!http)
    {
        free(url);
        failed("A HTTP kliens nem indítható.", 0);
        return NULL;
    }
    int error = esp_http_client_open(http, 0);
    bool ok = error == ESP_OK;
    if (!ok)
        failed("A HTTP kapcsolat nem épült fel.", error);
    if (ok)
    {
        int64_t size = esp_http_client_fetch_headers(http);
        int code = esp_http_client_get_status_code(http);
        ok = size >= 0 && code == 200;
        if (!ok)
            failed("A HTTP lekérés sikertelen (átirányítás nem követhető).",
                   size < 0 ? (int)size : code);
        else if ((uint64_t)size > limit)
        {
            ok = false;
            failed("A HTTP válasz meghaladja a megengedett méretet.", 0);
        }
    }
    size_t used = 0, capacity = limit < 4096 ? limit : 4096;
    char *body = ok ? malloc(capacity + 1) : NULL;
    if (ok && !body)
    {
        ok = false;
        failed("Nincs elég memória a HTTP válaszhoz.", 0);
    }
    while (ok)
    {
        if (used == capacity)
        {
            if (capacity == limit)
            {
                char extra;
                int count = esp_http_client_read(http, &extra, 1);
                if (count != 0)
                {
                    ok = false;
                    failed("A HTTP válasz túl nagy vagy hiányos.", 0);
                }
                break;
            }
            size_t next = capacity > limit / 2 ? limit : capacity * 2;
            char *larger = realloc(body, next + 1);
            if (!larger)
            {
                ok = false;
                failed("Nincs elég memória a HTTP válasz bővítéséhez.", 0);
                break;
            }
            body = larger;
            capacity = next;
        }
        int count = esp_http_client_read(http, body + used, capacity - used);
        if (count < 0)
        {
            ok = false;
            failed("A HTTP válasz olvasása megszakadt.", count);
            break;
        }
        if (count == 0)
            break;
        used += count;
    }
    if (ok && !esp_http_client_is_complete_data_received(http))
    {
        ok = false;
        failed("Hiányos HTTP válasz érkezett.", 0);
    }
    esp_http_client_close(http);
    esp_http_client_cleanup(http);
    free(url);
    if (!ok)
    {
        free(body);
        return NULL;
    }
    body[used] = 0;
    *length = used;
    return body;
}
static bool download_catalog_with_fallback(const lx_config_t *cfg, const char *token)
{
    int code = 0;
    if (download_catalog(&code))
        return true;
    if (code != 404 || s_dead)
        return false;
    phase("Loxone adatlista / HTTP tokenhitelesítés");
    size_t length = 0;
    char *body = http_body(cfg, "jdev/sys/getkey", 8192, &length);
    if (!body)
        return false;
    cJSON *root = cJSON_ParseWithLength(body, length);
    free(body);
    const cJSON *ll = cJSON_GetObjectItemCaseSensitive(root, "LL");
    const cJSON *status_code = cJSON_GetObjectItemCaseSensitive(ll, "Code");
    if (!status_code)
        status_code = cJSON_GetObjectItemCaseSensitive(ll, "code");
    int result = cJSON_IsNumber(status_code) ? status_code->valueint
               : cJSON_IsString(status_code) ? atoi(status_code->valuestring)
                                             : 0;
    cJSON *key =
        result == 200 ? cJSON_Duplicate(cJSON_GetObjectItemCaseSensitive(ll, "value"), true) : NULL;
    cJSON_Delete(root);
    if (!key)
        return failed("A HTTP hitelesítési kulcs nem érkezett meg.", result);
    if (cJSON_IsString(key))
    {
        cJSON *object = cJSON_CreateObject();
        if (object)
        {
            cJSON_AddStringToObject(object, "key", key->valuestring);
            cJSON_AddStringToObject(object, "hashAlg", "SHA1");
        }
        cJSON_Delete(key);
        key = object;
    }
    char hash[65];
    bool ok = key && hash_hmac(key, token, hash);
    cJSON_Delete(key);
    if (!ok)
        return failed("A HTTP tokenhitelesítés nem készíthető elő.", 0);
    char *user = urlencode(cfg->username);
    if (!user)
        return failed("Nincs elég memória a felhasználónévhez.", 0);
    char path[512];
    snprintf(path, sizeof(path), "data/LoxAPP3.json?autht=%s&user=%s", hash, user);
    free(user);
    phase("Loxone adatlista / HTTP letöltés");
    body = http_body(cfg, path, LX_STRUCTURE_MAX, &length);
    mbedtls_platform_zeroize(path, sizeof(path));
    if (!body)
        return false;
    cJSON *catalog = sm_lx_catalog(body, length);
    if (!catalog)
    {
        root = cJSON_ParseWithLength(body, length);
        ll = cJSON_GetObjectItemCaseSensitive(root, "LL");
        status_code = cJSON_GetObjectItemCaseSensitive(ll, "Code");
        if (!status_code)
            status_code = cJSON_GetObjectItemCaseSensitive(ll, "code");
        result = cJSON_IsNumber(status_code) ? status_code->valueint
               : cJSON_IsString(status_code) ? atoi(status_code->valuestring)
                                             : 0;
        cJSON_Delete(root);
        free(body);
        return failed(
            result ? "A Miniserver a HTTP adatlista kérését elutasította."
                   : "A HTTP adatlista nem dolgozható fel (JSON, állapot- vagy memóriakorlát).",
            result);
    }
    free(body);
    if (s_dead)
    {
        cJSON_Delete(catalog);
        return failed("A WebSocket kapcsolat megszakadt a HTTP letöltés közben.", 0);
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    cJSON_Delete(s_catalog);
    s_catalog = catalog;
    xSemaphoreGive(s_lock);
    return true;
}
static void close_connection(void)
{
    if (s_ws)
    {
        esp_websocket_client_stop(s_ws);
        esp_websocket_client_destroy(s_ws);
        s_ws = NULL;
    }
    free(s_public_key);
    s_public_key = NULL;
    s_salt[0] = 0;
    free(s_rx);
    s_rx = NULL;
    s_rx_len = s_rx_cap = 0;
    s_frame = (sm_lx_frame_t){0};
    message_t m;
    while (xQueueReceive(s_messages, &m, 0) == pdTRUE)
        free_message(&m);
    mbedtls_platform_zeroize(s_aes_key, sizeof(s_aes_key));
    mbedtls_platform_zeroize(s_iv, sizeof(s_iv));
}
/* A nyilvános kulcsot még a rövid WS hitelesítési határidő ELŐTT kérjük le. */
static bool fetch_public_key(const lx_config_t *cfg)
{
    phase("Miniserver elérése / nyilvános kulcs");
    char url[192];
    snprintf(url,
             sizeof(url),
             "%s://%s:%u/jdev/sys/getPublicKey",
             cfg->tls ? "https" : "http",
             cfg->host,
             (unsigned)cfg->port);
    esp_http_client_config_t options = {.url = url,
                                        .timeout_ms = LX_TIMEOUT_MS,
                                        .cert_pem = cfg->tls ? cfg->ca_pem : NULL,
                                        .disable_auto_redirect = true};
    esp_http_client_handle_t http = esp_http_client_init(&options);
    if (!http)
        return failed("A HTTP kliens nem indítható (memóriahiba).", 0);
    char *body = calloc(1, 8193);
    int http_result = body ? esp_http_client_open(http, 0) : ESP_ERR_NO_MEM;
    bool ok = http_result == ESP_OK;
    size_t used = 0;
    if (!ok)
        failed("Nem épült fel a HTTP kapcsolat. Ellenőrizd az IP-címet és a portot.", http_result);
    if (ok)
    {
        int64_t headers = esp_http_client_fetch_headers(http);
        int http_status = esp_http_client_get_status_code(http);
        ok = headers >= 0 && http_status == 200;
        if (!ok)
            failed("A HTTP kérés sikertelen. Ellenőrizd a portot és a WS/WSS módot.",
                   headers < 0 ? (int)headers : http_status);
    }
    while (ok && used < 8192)
    {
        int n = esp_http_client_read(http, body + used, 8192 - used);
        if (n < 0)
        {
            failed("A HTTP válasz olvasása megszakadt.", n);
            ok = false;
            break;
        }
        if (n == 0)
            break;
        used += n;
    }
    if (ok && (used == 8192 || !esp_http_client_is_complete_data_received(http)))
    {
        failed("Hiányos vagy túl nagy nyilvánoskulcs-válasz.", 0);
        ok = false;
    }
    esp_http_client_close(http);
    esp_http_client_cleanup(http);
    cJSON *root = ok ? cJSON_Parse(body) : NULL;
    free(body);
    const char *key = string(cJSON_GetObjectItemCaseSensitive(root, "LL"), "value");
    s_public_key = *key ? strdup(key) : NULL;
    cJSON_Delete(root);
    if (!s_public_key && ok)
        failed("A HTTP válasz nem tartalmaz Loxone nyilvános kulcsot.", 0);
    return s_public_key != NULL;
}
static bool connect_server(const lx_config_t *cfg)
{
    if (!fetch_public_key(cfg))
        return false;
    phase("Loxone WebSocket kapcsolat felépítése");
    char uri[192];
    snprintf(uri,
             sizeof(uri),
             "%s://%s:%u/ws/rfc6455",
             cfg->tls ? "wss" : "ws",
             cfg->host,
             (unsigned)cfg->port);
    esp_websocket_client_config_t ws = {.uri = uri,
                                        .subprotocol = "remotecontrol",
                                        .disable_auto_reconnect = true,
                                        .buffer_size = 2048,
                                        .task_stack = 5120,
                                        .network_timeout_ms = LX_TIMEOUT_MS,
                                        .cert_pem = cfg->tls ? cfg->ca_pem : NULL};
    s_rx_failure = LX_RX_OK;
    s_dead = false;
    s_ws = esp_websocket_client_init(&ws);
    if (!s_ws)
        return failed("A WebSocket kliens nem indítható (memóriahiba).", 0);
    if (esp_websocket_register_events(s_ws, WEBSOCKET_EVENT_ANY, receive_event, NULL) != ESP_OK ||
        esp_websocket_client_start(s_ws) != ESP_OK)
        return failed("A WebSocket feladat nem indítható.", 0);
    int64_t until = esp_timer_get_time() + LX_TIMEOUT_MS * 1000LL;
    while (!s_dead && !esp_websocket_client_is_connected(s_ws) && esp_timer_get_time() < until)
        vTaskDelay(pdMS_TO_TICKS(50));
    if (s_dead || !esp_websocket_client_is_connected(s_ws))
        return failed("A WebSocket nem kapcsolódott 12 másodpercen belül.", 0);
    return key_exchange();
}

static void worker(void *argument)
{
    lx_config_t *config = argument;
    char token[1024] = {0};
    unsigned delay_seconds = 2;
    while (true)
    {
        app_network_status_t network;
        app_network_status(&network);
        if (!network.connected)
        {
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        xSemaphoreTake(s_lock, portMAX_DELAY);
        sm_lx_cache_clear(&s_cache);
        s_live = false;
        xSemaphoreGive(s_lock);
        s_session_config = config;
        bool ok = connect_server(config) && acquire_token(config, token, sizeof(token)) &&
                  download_catalog_with_fallback(config, token);
        if (ok)
        {
            cJSON *response = reply("jdev/sps/enablebinstatusupdate", true);
            ok = response != NULL;
            cJSON_Delete(response);
        }
        if (ok)
        {
            xSemaphoreTake(s_lock, portMAX_DELAY);
            s_live = true;
            xSemaphoreGive(s_lock);
            status("Loxone kapcsolódva / Loxone connected");
            delay_seconds = 2;
            int64_t keepalive = esp_timer_get_time();
            while (!s_dead)
            {
                message_t message;
                if (xQueueReceive(s_messages, &message, pdMS_TO_TICKS(100)) == pdTRUE)
                {
                    mark_response();
                    if (!update_values(message.type, message.data, message.length))
                        s_dead = true;
                    free_message(&message);
                }
                int64_t now = esp_timer_get_time();
                if (now - s_last_response > 45000000LL)
                    s_dead = true;
                if (now - keepalive >= 15000000LL)
                {
                    if (!send_plain("keepalive"))
                        s_dead = true;
                    keepalive = now;
                }
                if (now >= s_token_refresh_us && !renew_token(config, token))
                    s_dead = true;
            }
        }
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_live = false;
        sm_lx_cache_clear(&s_cache);
        xSemaphoreGive(s_lock);
        report_failure("Loxone kapcsolat megszakadt / Loxone disconnected");
        close_connection();
        s_session_config = NULL;
        mbedtls_platform_zeroize(token, sizeof(token));
        vTaskDelay(pdMS_TO_TICKS(delay_seconds * 1000));
        delay_seconds = delay_seconds < 16 ? delay_seconds * 2 : 30;
    }
}

bool app_loxone_read(const char *uuid, double *value, void *context)
{
    (void)context;
    if (!s_lock)
        return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool valid = s_live && !s_dead && sm_lx_cache_read(&s_cache, uuid, value);
    xSemaphoreGive(s_lock);
    return valid;
}

bool app_loxone_connected(void)
{
    if (!s_lock)
        return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    bool live = s_live && !s_dead;
    xSemaphoreGive(s_lock);
    return live;
}

cJSON *app_loxone_catalog(void)
{
    if (!s_lock)
        return cJSON_CreateArray();
    xSemaphoreTake(s_lock, portMAX_DELAY);
    cJSON *result = s_catalog ? cJSON_Duplicate(s_catalog, true) : cJSON_CreateArray();
    xSemaphoreGive(s_lock);
    return result;
}

cJSON *app_loxone_live(void)
{
    cJSON *result = cJSON_CreateObject(), *values = cJSON_CreateObject();
    if (!result || !values)
    {
        cJSON_Delete(result);
        cJSON_Delete(values);
        return NULL;
    }
    if (s_lock)
        xSemaphoreTake(s_lock, portMAX_DELAY);
    bool live = s_lock && s_live && !s_dead;
    cJSON_AddBoolToObject(result, "connected", live);
    cJSON_AddStringToObject(result, "status", s_status);
    if (live)
        for (unsigned i = 0; i < SM_LX_CACHE_SLOTS; i++)
        {
            const sm_lx_entry_t *entry = &s_cache.entries[i];
            if (entry->uuid[0] && entry->valid)
                cJSON_AddNumberToObject(values, entry->uuid, entry->value);
        }
    if (s_lock)
        xSemaphoreGive(s_lock);
    cJSON_AddItemToObject(result, "values", values);
    return result;
}

esp_err_t app_loxone_start(void)
{
    s_lock = xSemaphoreCreateMutex();
    s_messages = xQueueCreate(LX_QUEUE_DEPTH, sizeof(message_t));
    if (!s_lock || !s_messages)
        return ESP_ERR_NO_MEM;
    lx_config_t *config = calloc(1, sizeof(*config));
    cJSON *settings = app_store_settings(false);
    if (!config || !settings)
    {
        free(config);
        cJSON_Delete(settings);
        return ESP_ERR_NO_MEM;
    }
    const cJSON *json = cJSON_GetObjectItemCaseSensitive(settings, "loxone");
    snprintf(config->host, sizeof(config->host), "%s", string(json, "host"));
    snprintf(config->username, sizeof(config->username), "%s", string(json, "username"));
    snprintf(config->password, sizeof(config->password), "%s", string(json, "password"));
    snprintf(config->ca_pem, sizeof(config->ca_pem), "%s", string(json, "ca_pem"));
    config->tls = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(json, "tls"));
    config->port = sm_json_number(json, "port", 80);
    cJSON_Delete(settings);
    if (!config->host[0] || !config->username[0])
    {
        free(config);
        return ESP_OK;
    }
    esp_sntp_config_t ntp = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    (void)esp_netif_sntp_init(&ntp);
    if (xTaskCreateWithCaps(
            worker, "schema_loxone", 32768, config, 4, NULL, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) !=
        pdPASS)
    {
        mbedtls_platform_zeroize(config, sizeof(*config));
        free(config);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}
