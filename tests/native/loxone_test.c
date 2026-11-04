#include "schema_loxone.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static unsigned calls;
static bool message(unsigned type, const unsigned char *data, size_t length, void *context)
{
    (void)context;
    assert(type == 6 || (type == 0 && length == 2 && !memcmp(data, "{}", 2)));
    calls++;
    return true;
}
int main(void)
{
    sm_lx_frame_t frame = {0};
    unsigned char header[] = {3, 0, 0x80, 0, 255, 255, 255, 255};
    assert(sm_lx_message(&frame, header, 8, message, NULL) && calls == 0 && !frame.pending);
    header[2] = 0;
    header[4] = 2;
    header[5] = header[6] = header[7] = 0;
    assert(sm_lx_message(&frame, header, 8, message, NULL));
    assert(!sm_lx_message(&frame, (const unsigned char *)"{", 1, message, NULL) && !frame.pending &&
           calls == 0);
    assert(sm_lx_message(&frame, header, 8, message, NULL));
    assert(sm_lx_message(&frame, (const unsigned char *)"{}", 2, message, NULL) && calls == 1);
    header[1] = 6;
    header[4] = 0;
    assert(sm_lx_message(&frame, header, 8, message, NULL) && calls == 2);
    header[4] = header[5] = header[6] = header[7] = 255;
    assert(!sm_lx_message(&frame, header, 8, message, NULL));
    const char *source =
        "{\"rooms\":{\"r\":{\"name\":\"Gépház\"}},\"cats\":{\"c\":{\"name\":\"Fűtés\"}},"
        "\"controls\":{\"pump\":{\"name\":\"Szivattyú\",\"room\":\"r\",\"cat\":\"c\",\"states\":{"
        "\"active\":\"12345678-1234-5678-0102030405060708\"},\"subControls\":{\"sub\":{\"name\":"
        "\"Teljesítmény\",\"states\":{\"value\":\"22345678-1234-5678-0102030405060708\"}}}},"
        "\"button\":{\"uuidAction\":\"33345678-1234-5678-0102030405060708\"}}}";
    cJSON *catalog = sm_lx_catalog(source, strlen(source));
    assert(catalog && cJSON_GetArraySize(catalog) == 2);
    assert(!strcmp(sm_json_string(cJSON_GetArrayItem(catalog, 1), "room"), "Gépház"));
    assert(!strcmp(sm_json_string(cJSON_GetArrayItem(catalog, 1), "category"), "Fűtés"));
    cJSON_Delete(catalog);
    assert(!sm_lx_catalog("{}", 2));
    puts("Loxone: exact/estimated headers, truncation, limits, keepalive, read-only catalogue and "
         "inherited filters OK");
}
