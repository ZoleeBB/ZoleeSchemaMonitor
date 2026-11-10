#include "schema_model.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A raszternév egyben a firmware által elfogadott vászonméretet is kijelöli. */
unsigned sm_manifold_raster_circuits(const char *id)
{
    if (!strcmp(id, "configurable-manifold"))
        return 4;
    if (strncmp(id, "manifold-", 9))
        return 0;
    const char *digits = id + 9;
    if (!strncmp(digits, "cool-", 5) || !strncmp(digits, "body-", 5))
        digits += 5;
    else if (!sm_asset_find(id))
        return 0;
    if (*digits < '1' || *digits > '9' || strlen(digits) > 2 ||
        digits[strspn(digits, "0123456789")])
        return 0;
    unsigned n = (unsigned)atoi(digits);
    return n >= 2 && n <= 16 ? n : 0;
}

cJSON *sm_json_parse(const char *data, size_t length)
{
    return sm_json_parse_limit(data, length, SM_PROJECT_BYTES + 32768U);
}

cJSON *sm_json_parse_limit(const char *data, size_t length, size_t limit)
{
    if (!data || !length || length > limit || limit > 2U * SM_PROJECT_BYTES + 131072U)
        return NULL;
    bool quoted = false;
    int depth = 0;
    for (size_t i = 0; i < length; i++)
    {
        unsigned char c = (unsigned char)data[i];
        if (!c)
            return NULL;
        /* A cJSON a \\u0000 karaktert C-sztring-véggé alakítaná. */
        if (c == '\\')
        {
            if (++i >= length)
                return NULL;
            if (data[i] == 'u' && i + 4 < length && !memcmp(data + i + 1, "0000", 4))
                return NULL;
            continue;
        }
        if (c == '"')
            quoted = !quoted;
        if (quoted && c < 32)
            return NULL;
        if (!quoted && (c == '{' || c == '[') && ++depth > 32)
            return NULL;
        if (!quoted && (c == '}' || c == ']') && --depth < 0)
            return NULL;
        if (c < 0x80)
            continue;
        unsigned count = c >= 0xc2 && c <= 0xdf ? 1
                       : c >= 0xe0 && c <= 0xef ? 2
                       : c >= 0xf0 && c <= 0xf4 ? 3
                                                : 0;
        if (!count || i + count >= length)
            return NULL;
        unsigned char next = (unsigned char)data[i + 1];
        if ((c == 0xe0 && next < 0xa0) || (c == 0xed && next >= 0xa0) ||
            (c == 0xf0 && next < 0x90) || (c == 0xf4 && next >= 0x90))
            return NULL;
        while (count--)
            if (((unsigned char)data[++i] & 0xc0) != 0x80)
                return NULL;
    }
    const char *end = NULL;
    cJSON *result = cJSON_ParseWithLengthOpts(data, length, &end, false);
    if (!result)
        return NULL;
    while (end < data + length && (*end == ' ' || *end == '\t' || *end == '\r' || *end == '\n'))
        end++;
    if (end != data + length)
    {
        cJSON_Delete(result);
        return NULL;
    }
    return result;
}

const char *sm_json_string(const cJSON *object, const char *key)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key);
    return cJSON_IsString(value) ? value->valuestring : "";
}

double sm_json_number(const cJSON *object, const char *key, double fallback)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key);
    return cJSON_IsNumber(value) ? value->valuedouble : fallback;
}

static bool fail(char *error, const char *message)
{
    snprintf(error, SM_ERROR_SIZE, "%s", message);
    return false;
}

/* Ismeretlen vagy ismétlődő mező nem kerülhet mentett konfigurációba. */
static bool fields(const cJSON *object, const char *allowed)
{
    if (!cJSON_IsObject(object))
        return false;
    const cJSON *entry;
    cJSON_ArrayForEach(entry, object)
    {
        if (!entry->string || strlen(entry->string) > 63 || strchr(entry->string, '|'))
            return false;
        char key[68];
        snprintf(key, sizeof(key), "|%s|", entry->string);
        if (!strstr(allowed, key))
            return false;
        for (const cJSON *other = entry->next; other; other = other->next)
            if (other->string && !strcmp(other->string, entry->string))
                return false;
    }
    return true;
}

static bool number(const cJSON *object, const char *key, double minimum, double maximum)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key);
    return cJSON_IsNumber(value) && isfinite(value->valuedouble) && value->valuedouble >= minimum &&
           value->valuedouble <= maximum;
}

static bool integer(const cJSON *object, const char *key, int minimum, int maximum)
{
    return number(object, key, minimum, maximum) &&
           floor(sm_json_number(object, key, NAN)) == sm_json_number(object, key, NAN);
}

static bool text(const cJSON *object, const char *key, size_t maximum)
{
    const cJSON *value = cJSON_GetObjectItemCaseSensitive(object, key);
    if (!cJSON_IsString(value) || !value->valuestring)
        return false;
    /* UTF-16 egységszám: a JS String.length korlátjának felel meg. */
    size_t count = 0;
    for (const unsigned char *p = (const unsigned char *)value->valuestring; *p; p++)
    {
        if (*p < 32 && *p != '\n' && *p != '\r' && *p != '\t')
            return false;
        if ((*p & 0xc0) != 0x80)
            count += *p >= 0xf0 ? 2 : 1;
        if (count > maximum)
            return false;
    }
    return true;
}

static bool identifier(const char *id)
{
    if (!*id || strlen(id) > 64)
        return false;
    for (const unsigned char *p = (const unsigned char *)id; *p; p++)
        if (!((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') || (*p >= '0' && *p <= '9') ||
              *p == '-' || *p == '_'))
            return false;
    return true;
}

bool sm_binding_validate(const cJSON *binding)
{
    return cJSON_IsNull(binding) || (fields(binding, "|uuid|scale|offset|") &&
                                     sm_uuid_valid(sm_json_string(binding, "uuid")) &&
                                     number(binding, "scale", -1000000, 1000000) &&
                                     number(binding, "offset", -1000000, 1000000));
}

bool sm_binding_parse(const cJSON *json, sm_binding_t *binding)
{
    memset(binding, 0, sizeof(*binding));
    if (!json || !sm_binding_validate(json) || cJSON_IsNull(json))
        return false;
    snprintf(binding->uuid, sizeof(binding->uuid), "%s", sm_json_string(json, "uuid"));
    for (char *p = binding->uuid; *p; p++)
        *p = (char)tolower((unsigned char)*p);
    binding->scale = sm_json_number(json, "scale", 1);
    binding->offset = sm_json_number(json, "offset", 0);
    return true;
}

sm_signal_t sm_binding_read(const sm_binding_t *binding, bool valid, double raw)
{
    sm_signal_t signal = {.configured = binding && binding->uuid[0]};
    if (signal.configured && valid)
    {
        signal.value = raw * binding->scale + binding->offset;
        signal.valid = isfinite(signal.value);
    }
    return signal;
}

static bool element_valid(const cJSON *element, char *error)
{
    if (!fields(element,
                "|id|type|asset|x|y|w|h|rotation|label|text|color|fontSize|unit|decimals|value|"
                "activity|power|showPower|points|width|direction|source|target|position|manifold|align|"))
        return fail(error, "element.fields");
    const char *type = sm_json_string(element, "type");
    if (!identifier(sm_json_string(element, "id")) ||
        (strcmp(type, "equipment") && strcmp(type, "text") && strcmp(type, "value") &&
         strcmp(type, "pipe")))
        return fail(error, "element.id/type");
    if (!text(element, "asset", 64) ||
        (!strcmp(type, "equipment") && !sm_asset_find(sm_json_string(element, "asset"))))
        return fail(error, "element.asset");
    if (!number(element, "x", 0, 1) || !number(element, "y", 0, 1) ||
        !number(element, "w", .005, 1) || !number(element, "h", .005, 1) ||
        sm_json_number(element, "x", 0) + sm_json_number(element, "w", 0) > 1.000001 ||
        sm_json_number(element, "y", 0) + sm_json_number(element, "h", 0) > 1.000001)
        return fail(error, "element.bounds");
    if (!integer(element, "rotation", 0, 270) ||
        !sm_rotation_valid((unsigned)sm_json_number(element, "rotation", 999)))
        return fail(error, "element.rotation");
    if (!text(element, "label", 120) || !text(element, "text", 1000) || !text(element, "unit", 24))
        return fail(error, "element.text");
    const cJSON *alignment = cJSON_GetObjectItemCaseSensitive(element, "align");
    if (alignment &&
        ((strcmp(type, "text") && strcmp(type, "value")) || !cJSON_IsString(alignment) ||
         (strcmp(alignment->valuestring, "left") && strcmp(alignment->valuestring, "center") &&
          strcmp(alignment->valuestring, "right"))))
        return fail(error, "element.align");
    const char *color = sm_json_string(element, "color");
    if (strlen(color) != 7 || color[0] != '#')
        return fail(error, "element.color");
    for (unsigned i = 1; i < 7; i++)
        if (!isxdigit((unsigned char)color[i]))
            return fail(error, "element.color");
    if (!number(element, "fontSize", 10, 64) || !integer(element, "decimals", 0, 6) ||
        !cJSON_IsBool(cJSON_GetObjectItemCaseSensitive(element, "showPower")))
        return fail(error, "element.format");
    if (!sm_binding_validate(cJSON_GetObjectItemCaseSensitive(element, "value")) ||
        !sm_binding_validate(cJSON_GetObjectItemCaseSensitive(element, "activity")))
        return fail(error, "element.binding");
    const cJSON *power = cJSON_GetObjectItemCaseSensitive(element, "power");
    if (!cJSON_IsNull(power) &&
        (!fields(power, "|source|minimum|maximum|") ||
         !sm_binding_validate(cJSON_GetObjectItemCaseSensitive(power, "source")) ||
         cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(power, "source")) ||
         !number(power, "minimum", -1e12, 1e12) || !number(power, "maximum", -1e12, 1e12) ||
         !sm_power_range_valid(sm_json_number(power, "minimum", NAN),
                               sm_json_number(power, "maximum", NAN))))
        return fail(error, "power.range");
    /* Hiányzó állásmező a korábbi mentésekben megengedett. */
    const cJSON *position = cJSON_GetObjectItemCaseSensitive(element, "position");
    if (position && !cJSON_IsNull(position))
    {
        if (strcmp(type, "equipment") || !fields(position, "|source|minimum|maximum|reverse|") ||
            !sm_binding_validate(cJSON_GetObjectItemCaseSensitive(position, "source")) ||
            cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(position, "source")) ||
            !number(position, "minimum", -1e12, 1e12) ||
            !number(position, "maximum", -1e12, 1e12) ||
            !sm_power_range_valid(sm_json_number(position, "minimum", NAN),
                                 sm_json_number(position, "maximum", NAN)) ||
            !cJSON_IsBool(cJSON_GetObjectItemCaseSensitive(position, "reverse")))
            return fail(error, "position.range");
    }
    const cJSON *manifold = cJSON_GetObjectItemCaseSensitive(element, "manifold");
    if (!strcmp(type, "equipment") &&
        !strcmp(sm_json_string(element, "asset"), "configurable-manifold"))
    {
        const char *variant = sm_json_string(manifold, "variant");
        const cJSON *heads = cJSON_GetObjectItemCaseSensitive(manifold, "heads");
        if (!fields(manifold, "|circuits|variant|connections|heads|") ||
            !integer(manifold, "circuits", 2, 16) ||
            (strcmp(variant, "heating") && strcmp(variant, "floor") &&
             strcmp(variant, "ceiling-heat") && strcmp(variant, "ceiling-cool")) ||
            !cJSON_IsBool(cJSON_GetObjectItemCaseSensitive(manifold, "connections")) ||
            !cJSON_IsArray(heads) ||
            cJSON_GetArraySize(heads) != sm_json_number(manifold, "circuits", 0))
            return fail(error, "manifold");
        const cJSON *head;
        cJSON_ArrayForEach(head, heads) if (!fields(head, "|percent|source|") ||
                                            !number(head, "percent", 0, 100) ||
                                            !sm_binding_validate(cJSON_GetObjectItemCaseSensitive(
                                                head, "source"))) return fail(error,
                                                                              "manifold.head");
    }
    else if (manifold)
        return fail(error, "manifold.asset");
    if (!strcmp(type, "pipe"))
    {
        const cJSON *points = cJSON_GetObjectItemCaseSensitive(element, "points");
        if (!cJSON_IsArray(points) || cJSON_GetArraySize(points) < 2 ||
            cJSON_GetArraySize(points) > SM_MAX_POINTS || !number(element, "width", 1, 30) ||
            (sm_json_number(element, "direction", 0) != -1 &&
             sm_json_number(element, "direction", 0) != 1))
            return fail(error, "pipe.points");
        const cJSON *point;
        cJSON_ArrayForEach(point, points)
        {
            if (!fields(point, "|x|y|link|kind|") || !number(point, "x", 0, 1) ||
                !number(point, "y", 0, 1))
                return fail(error, "pipe.point");
            if (!cJSON_HasObjectItem(point, "link") && !cJSON_HasObjectItem(point, "kind"))
                continue;
            const char *kind = sm_json_string(point, "kind");
            if (!identifier(sm_json_string(point, "link")) ||
                (strcmp(kind, "node") && strcmp(kind, "over") && strcmp(kind, "under")))
                return fail(error, "pipe.link");
            if (strcmp(kind, "node") && (point == points->child || !point->next))
                return fail(error, "pipe.bridge");
        }
    }
    return true;
}

typedef struct
{
    const cJSON *pipe, *point;
    const char *link;
    unsigned owner;
} pipe_member_t;

static int compare_members(const void *left, const void *right)
{
    const pipe_member_t *a = left, *b = right;
    int order = strcmp(a->link, b->link);
    return order ? order : (int)a->owner - (int)b->owner;
}

static bool same_pipe_flow(const cJSON *a, const cJSON *b)
{
    const char *keys[] = {"activity", "power", "direction"};
    for (unsigned i = 0; i < 3; i++)
        if (!cJSON_Compare(cJSON_GetObjectItemCaseSensitive(a, keys[i]),
                           cJSON_GetObjectItemCaseSensitive(b, keys[i]), true))
            return false;
    return true;
}

static bool valid_pipe_links(const cJSON *elements)
{
    const cJSON *pipe, *point;
    size_t count = 0;
    cJSON_ArrayForEach(pipe, elements)
        if (!strcmp(sm_json_string(pipe, "type"), "pipe"))
            cJSON_ArrayForEach(point, cJSON_GetObjectItemCaseSensitive(pipe, "points"))
                if (cJSON_HasObjectItem(point, "link"))
                    count++;
    if (!count)
        return true;
    pipe_member_t *members = calloc(count, sizeof(*members));
    if (!members)
        return false;
    size_t used = 0;
    unsigned owner = 0;
    cJSON_ArrayForEach(pipe, elements)
    {
        if (!strcmp(sm_json_string(pipe, "type"), "pipe"))
            cJSON_ArrayForEach(point, cJSON_GetObjectItemCaseSensitive(pipe, "points"))
                if (cJSON_HasObjectItem(point, "link"))
                    members[used++] = (pipe_member_t){pipe, point, sm_json_string(point, "link"), owner};
        owner++;
    }
    qsort(members, count, sizeof(*members), compare_members);
    bool valid = true;
    for (size_t i = 0; valid && i < count;)
    {
        size_t start = i;
        const pipe_member_t *first = &members[start];
        unsigned nodes = 0, overs = 0, unders = 0;
        do
        {
            const pipe_member_t *m = &members[i];
            const char *kind = sm_json_string(m->point, "kind");
            if (!strcmp(kind, "node"))
            {
                nodes++;
                valid = valid && same_pipe_flow(first->pipe, m->pipe);
            }
            else if (!strcmp(kind, "over"))
                overs++;
            else
                unders++;
            valid = valid && (i == start || members[i-1].owner != m->owner) &&
                fabs(sm_json_number(m->point,"x",0) - sm_json_number(first->point,"x",0)) <= 1e-9 &&
                fabs(sm_json_number(m->point,"y",0) - sm_json_number(first->point,"y",0)) <= 1e-9;
            i++;
        } while (i < count && !strcmp(members[i].link, first->link));
        valid = valid && ((nodes >= 2 && !overs && !unders) || (!nodes && overs == 1 && unders == 1));
    }
    free(members);
    return valid;
}

bool sm_project_validate(const cJSON *project, char error[SM_ERROR_SIZE])
{
    error[0] = 0;
    if (!fields(project, "|format|version|title|pages|") ||
        strcmp(sm_json_string(project, "format"), "zolee-schema") ||
        !integer(project, "version", 1, 1) || !text(project, "title", 80))
        return fail(error, "project.format/version");
    const cJSON *pages = cJSON_GetObjectItemCaseSensitive(project, "pages");
    if (!cJSON_IsArray(pages) || cJSON_GetArraySize(pages) < 1 ||
        cJSON_GetArraySize(pages) > SM_MAX_PAGES)
        return fail(error, "project.pages");
    const cJSON *page;
    cJSON_ArrayForEach(page, pages)
    {
        if (!fields(page, "|id|name|background|elements|") ||
            !identifier(sm_json_string(page, "id")) || !text(page, "name", 80))
            return fail(error, "page.fields");
        const char *background = sm_json_string(page, "background");
        if (strcmp(background, "plain") && strcmp(background, "cool") && strcmp(background, "warm"))
            return fail(error, "page.background");
        for (const cJSON *other = page->next; other; other = other->next)
            if (!strcmp(sm_json_string(page, "id"), sm_json_string(other, "id")))
                return fail(error, "page.duplicate");
        const cJSON *elements = cJSON_GetObjectItemCaseSensitive(page, "elements");
        if (!cJSON_IsArray(elements) || cJSON_GetArraySize(elements) > SM_MAX_ELEMENTS)
            return fail(error, "page.elements");
        const cJSON *element;
        cJSON_ArrayForEach(element, elements)
        {
            if (!element_valid(element, error))
                return false;
            for (const cJSON *other = element->next; other; other = other->next)
                if (!strcmp(sm_json_string(element, "id"), sm_json_string(other, "id")))
                    return fail(error, "element.duplicate");
        }
        if (!valid_pipe_links(elements))
            return fail(error, "pipe.link");
        cJSON_ArrayForEach(element, elements)
        {
            if (strcmp(sm_json_string(element, "type"), "pipe"))
                continue;
            const char *ends[] = {"source", "target"};
            for (unsigned i = 0; i < 2; i++)
            {
                const cJSON *end = cJSON_GetObjectItemCaseSensitive(element, ends[i]);
                if (cJSON_IsNull(end))
                    continue;
                if (!fields(end, "|element|port|") || !identifier(sm_json_string(end, "element")) ||
                    !integer(end, "port", 0, 33))
                    return fail(error, "pipe.endpoint");
                unsigned port_count = 0;
                const sm_asset_t *asset = NULL;
                const cJSON *other;
                cJSON_ArrayForEach(
                    other, elements) if (!strcmp(sm_json_string(other, "id"),
                                                 sm_json_string(end, "element")) &&
                                         !strcmp(sm_json_string(other, "type"), "equipment"))
                {
                    asset = sm_asset_find(sm_json_string(other, "asset"));
                    const cJSON *m = cJSON_GetObjectItemCaseSensitive(other, "manifold");
                    port_count =
                        m ? (cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(m, "connections"))
                                 ? 2 + 2 * sm_json_number(m, "circuits", 0)
                                 : 0)
                          : (asset ? asset->port_count : 0);
                }
                if (!asset || sm_json_number(end, "port", 999) >= port_count)
                    return fail(error, "pipe.endpoint");
            }
        }
    }
    char *encoded = cJSON_PrintUnformatted(project);
    bool size_valid = encoded && strlen(encoded) <= SM_PROJECT_BYTES;
    free(encoded);
    return size_valid || fail(error, "project.size");
}

bool sm_settings_validate(const cJSON *settings, char error[SM_ERROR_SIZE])
{
    error[0] = 0;
    if (!fields(settings, "|project|display|wifi|loxone|"))
        return fail(error, "settings.fields");
    const cJSON *project = cJSON_GetObjectItemCaseSensitive(settings, "project");
    const cJSON *display = cJSON_GetObjectItemCaseSensitive(settings, "display");
    const cJSON *wifi = cJSON_GetObjectItemCaseSensitive(settings, "wifi");
    const cJSON *loxone = cJSON_GetObjectItemCaseSensitive(settings, "loxone");
    if (!fields(project, "|title|language|") || !text(project, "title", 80) ||
        (strcmp(sm_json_string(project, "language"), "hu") &&
         strcmp(sm_json_string(project, "language"), "en")))
        return fail(error, "settings.project");
    if (!fields(display, "|rotation|dayBrightness|nightBrightness|nightSource|") ||
        !integer(display, "rotation", 0, 270) ||
        !sm_rotation_valid((unsigned)sm_json_number(display, "rotation", 999)) ||
        !integer(display, "dayBrightness", 1, 100) ||
        !integer(display, "nightBrightness", 0, 100) ||
        !sm_binding_validate(cJSON_GetObjectItemCaseSensitive(display, "nightSource")))
        return fail(error, "settings.display");
    if (!fields(wifi, "|ssid|password|") || !text(wifi, "ssid", 32) ||
        !text(wifi, "password", 64) || strlen(sm_json_string(wifi, "ssid")) > 32 ||
        strlen(sm_json_string(wifi, "password")) > 64)
        return fail(error, "settings.wifi");
    size_t password_length = strlen(sm_json_string(wifi, "password"));
    if (password_length > 0 && password_length < 8)
        return fail(error, "settings.wifi.password");
    if (password_length == 64)
        for (const char *p = sm_json_string(wifi, "password"); *p; p++)
            if (!isxdigit((unsigned char)*p))
                return fail(error, "settings.wifi.password");
    if (!fields(loxone, "|host|port|tls|ca_pem|username|password|") || !text(loxone, "host", 127) ||
        !integer(loxone, "port", 1, 65535) ||
        !cJSON_IsBool(cJSON_GetObjectItemCaseSensitive(loxone, "tls")) ||
        !text(loxone, "ca_pem", 4095) || strlen(sm_json_string(loxone, "ca_pem")) > 4095 ||
        !text(loxone, "username", 95) || strlen(sm_json_string(loxone, "username")) > 95 ||
        !text(loxone, "password", 127) || strlen(sm_json_string(loxone, "password")) > 127)
        return fail(error, "settings.loxone");
    for (const unsigned char *p = (const unsigned char *)sm_json_string(loxone, "host"); *p; p++)
        if (!isalnum(*p) && *p != '.' && *p != '-')
            return fail(error, "settings.host");
    if (cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(loxone, "tls")) &&
        !strstr(sm_json_string(loxone, "ca_pem"), "-----BEGIN CERTIFICATE-----"))
        return fail(error, "settings.ca");
    return true;
}

cJSON *sm_settings_defaults(void)
{
    return cJSON_Parse(
        "{\"project\":{\"title\":\"ZoleeSchemaMonitor\",\"language\":\"hu\"},"
        "\"display\":{\"rotation\":270,\"dayBrightness\":85,\"nightBrightness\":15,\"nightSource\":"
        "null},"
        "\"wifi\":{\"ssid\":\"\",\"password\":\"\"},\"loxone\":{\"host\":\"\",\"port\":80,\"tls\":"
        "false,\"ca_pem\":\"\",\"username\":\"\",\"password\":\"\"}}");
}

/* Érvénytelen lapindexnél az első lapra áll vissza a megjelenítő. */
cJSON *sm_project_page_copy(const cJSON *project, int index, int *count)
{
    const cJSON *pages = cJSON_GetObjectItemCaseSensitive(project, "pages");
    *count = cJSON_GetArraySize(pages);
    if (index < 0 || index >= *count)
        index = 0;
    return cJSON_Duplicate(cJSON_GetArrayItem(pages, index), true);
}

cJSON *sm_project_defaults(void)
{
    return cJSON_Parse(
        "{\"format\":\"zolee-schema\",\"version\":1,\"title\":\"ZoleeSchemaMonitor\","
        "\"pages\":[{\"id\":\"page-1\",\"name\":\"Gépészet\",\"background\":\"cool\",\"elements\":["
        "]}]}");
}
