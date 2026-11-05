/* Verziózott, hordozható AES-256-GCM mentés. A kulcsszármaztatás a Mbed TLS
 * PBKDF2-HMAC-SHA256 megvalósítása; saját kriptográfiai primitív nincs. */
#include "app_backup.h"
#include "esp_random.h"
#include "mbedtls/base64.h"
#include "mbedtls/gcm.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/platform_util.h"
#include <stdlib.h>
#include <string.h>
#ifdef ESP_PLATFORM
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif
#define ITERATIONS 600000U
static const unsigned char s_aad[] = "ZoleeSchemaMonitor backup v1";

static void wipe(cJSON *json)
{
    for (cJSON *entry = json; entry; entry = entry->next)
    {
        wipe(entry->child);
        if (cJSON_IsString(entry) && entry->valuestring)
            mbedtls_platform_zeroize(entry->valuestring, strlen(entry->valuestring));
    }
}
void app_backup_delete(cJSON *json)
{
    wipe(json);
    cJSON_Delete(json);
}

bool app_backup_passphrase_valid(const char *passphrase)
{
    if (!passphrase)
        return false;
    size_t bytes = strlen(passphrase), characters = 0;
    if (bytes > 128)
        return false;
    for (size_t i = 0; i < bytes; i++)
        if (((unsigned char)passphrase[i] & 0xc0) != 0x80)
            characters++;
    return characters >= 12;
}
static int derive(const char *passphrase, const unsigned char salt[16], unsigned char key[32])
{
#ifdef ESP_PLATFORM
    /* A hosszabb KDF alatt a főciklus, a Loxone és az idle task is futhat.
     * Itt sem állapottár-, sem kijelzőzárat nem tartunk. */
    UBaseType_t priority = uxTaskPriorityGet(NULL);
    vTaskPrioritySet(NULL, tskIDLE_PRIORITY);
#endif
    int result = mbedtls_pkcs5_pbkdf2_hmac_ext(MBEDTLS_MD_SHA256,
                                               (const unsigned char *)passphrase,
                                               strlen(passphrase),
                                               salt,
                                               16,
                                               ITERATIONS,
                                               32,
                                               key);
#ifdef ESP_PLATFORM
    vTaskPrioritySet(NULL, priority);
#endif
    return result;
}
static char *hex(const unsigned char *data, size_t size)
{
    const char *digits = "0123456789abcdef";
    char *text = malloc(size * 2 + 1);
    if (!text)
        return NULL;
    for (size_t i = 0; i < size; i++)
    {
        text[i * 2] = digits[data[i] >> 4];
        text[i * 2 + 1] = digits[data[i] & 15];
    }
    text[size * 2] = 0;
    return text;
}
static bool unhex(const char *text, unsigned char *data, size_t size)
{
    static const char digits[] = "0123456789abcdef";
    if (strlen(text) != size * 2)
        return false;
    for (size_t i = 0; i < size; i++)
    {
        const char *a = strchr(digits, text[i * 2]);
        const char *b = strchr(digits, text[i * 2 + 1]);
        if (!a || !b)
            return false;
        data[i] = (unsigned char)(((a - digits) << 4) | (b - digits));
    }
    return true;
}
cJSON *app_backup_seal(const cJSON *state, const char *passphrase)
{
    if (!cJSON_IsObject(state) || !app_backup_passphrase_valid(passphrase))
        return NULL;
    char *plain = cJSON_PrintUnformatted(state);
    size_t size = plain ? strlen(plain) : 0;
    if (!size || size > APP_BACKUP_PLAIN_BYTES)
    {
        if (plain)
            mbedtls_platform_zeroize(plain, size);
        free(plain);
        return NULL;
    }
    unsigned char salt[16], nonce[12], tag[16], key[32] = {0};
    esp_fill_random(salt, sizeof(salt));
    esp_fill_random(nonce, sizeof(nonce));
    unsigned char *cipher = malloc(size);
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int result = cipher ? derive(passphrase, salt, key) : -1;
    if (!result)
        result = mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, key, 256);
    if (!result)
        result = mbedtls_gcm_crypt_and_tag(&context,
                                           MBEDTLS_GCM_ENCRYPT,
                                           size,
                                           nonce,
                                           sizeof(nonce),
                                           s_aad,
                                           sizeof(s_aad) - 1,
                                           (unsigned char *)plain,
                                           cipher,
                                           sizeof(tag),
                                           tag);
    mbedtls_gcm_free(&context);
    mbedtls_platform_zeroize(key, sizeof(key));
    mbedtls_platform_zeroize(plain, size);
    free(plain);
    size_t encoded_size = 0;
    if (!result)
        (void)mbedtls_base64_encode(NULL, 0, &encoded_size, cipher, size);
    unsigned char *encoded = encoded_size ? malloc(encoded_size) : NULL;
    if (!result)
        result = encoded ? mbedtls_base64_encode(encoded, encoded_size, &encoded_size, cipher, size)
                         : -1;
    free(cipher);
    cJSON *envelope = NULL;
    char *salt_text = NULL, *nonce_text = NULL, *tag_text = NULL;
    if (!result && (salt_text = hex(salt, sizeof(salt))) &&
        (nonce_text = hex(nonce, sizeof(nonce))) && (tag_text = hex(tag, sizeof(tag))))
    {
        envelope = cJSON_CreateObject();
        bool ok = envelope && cJSON_AddStringToObject(envelope, "format", "zolee-backup") &&
                  cJSON_AddNumberToObject(envelope, "version", 1) &&
                  cJSON_AddStringToObject(envelope, "kdf", "PBKDF2-SHA256") &&
                  cJSON_AddNumberToObject(envelope, "iterations", ITERATIONS) &&
                  cJSON_AddStringToObject(envelope, "salt", salt_text) &&
                  cJSON_AddStringToObject(envelope, "nonce", nonce_text) &&
                  cJSON_AddStringToObject(envelope, "tag", tag_text) &&
                  cJSON_AddStringToObject(envelope, "data", (const char *)encoded);
        if (!ok)
        {
            cJSON_Delete(envelope);
            envelope = NULL;
        }
    }
    free(encoded);
    free(salt_text);
    free(nonce_text);
    free(tag_text);
    return envelope;
}
cJSON *app_backup_open(const cJSON *envelope, const char *passphrase)
{
    if (!app_backup_passphrase_valid(passphrase) || !cJSON_IsObject(envelope) ||
        cJSON_GetArraySize(envelope) != 8 ||
        strcmp(sm_json_string(envelope, "format"), "zolee-backup") ||
        sm_json_number(envelope, "version", 0) != 1 ||
        strcmp(sm_json_string(envelope, "kdf"), "PBKDF2-SHA256") ||
        sm_json_number(envelope, "iterations", 0) != ITERATIONS)
        return NULL;
    unsigned char salt[16], nonce[12], tag[16], key[32] = {0};
    if (!unhex(sm_json_string(envelope, "salt"), salt, sizeof(salt)) ||
        !unhex(sm_json_string(envelope, "nonce"), nonce, sizeof(nonce)) ||
        !unhex(sm_json_string(envelope, "tag"), tag, sizeof(tag)))
        return NULL;
    const unsigned char *encoded = (const unsigned char *)sm_json_string(envelope, "data");
    size_t encoded_size = strlen((const char *)encoded), size = 0;
    if (!encoded_size || encoded_size > 4 * ((APP_BACKUP_PLAIN_BYTES + 2) / 3) ||
        mbedtls_base64_decode(NULL, 0, &size, encoded, encoded_size) !=
            MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL ||
        !size || size > APP_BACKUP_PLAIN_BYTES)
        return NULL;
    size_t capacity = size;
    unsigned char *cipher = malloc(capacity), *plain = malloc(capacity);
    mbedtls_gcm_context context;
    mbedtls_gcm_init(&context);
    int result =
        cipher && plain ? mbedtls_base64_decode(cipher, size, &size, encoded, encoded_size) : -1;
    if (!result)
        result = derive(passphrase, salt, key);
    if (!result)
        result = mbedtls_gcm_setkey(&context, MBEDTLS_CIPHER_ID_AES, key, 256);
    if (!result)
        result = mbedtls_gcm_auth_decrypt(&context,
                                          size,
                                          nonce,
                                          sizeof(nonce),
                                          s_aad,
                                          sizeof(s_aad) - 1,
                                          tag,
                                          sizeof(tag),
                                          cipher,
                                          plain);
    cJSON *state = !result ? sm_json_parse((char *)plain, size) : NULL;
    mbedtls_gcm_free(&context);
    mbedtls_platform_zeroize(key, sizeof(key));
    if (plain)
        mbedtls_platform_zeroize(plain, capacity);
    free(cipher);
    free(plain);
    return state;
}
