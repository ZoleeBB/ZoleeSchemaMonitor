#include "schema_loxone.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

bool sm_lx_message(sm_lx_frame_t *frame,
                   const unsigned char *data,
                   size_t length,
                   sm_lx_message_fn callback,
                   void *context)
{
    if (!frame || !callback || (!data && length))
        return false;
    if (frame->pending)
    {
        unsigned type = frame->type;
        size_t expected = frame->length;
        *frame = (sm_lx_frame_t){0};
        return length == expected && callback(type, data, length, context);
    }
    if (length != 8 || data[0] != 3 || data[1] > 7 || (data[2] & ~0x80U) || data[3])
        return false;
    /* Becsült fejlécből nem foglalunk és nem küldünk részleges üzenetet. */
    if (data[2] & 0x80)
        return true;
    uint32_t declared = (uint32_t)data[4] | (uint32_t)data[5] << 8 | (uint32_t)data[6] << 16 |
                        (uint32_t)data[7] << 24;
    if (declared > SM_LX_STRUCTURE_BYTES + 1024)
        return false;
    if (!declared)
        return callback(data[1], NULL, 0, context);
    frame->pending = true;
    frame->type = data[1];
    frame->length = declared;
    return true;
}

static const char *group_name(const cJSON *groups, const char *id)
{
    return sm_json_string(cJSON_GetObjectItemCaseSensitive(groups, id), "name");
}

static bool controls(cJSON *out,
                     const cJSON *items,
                     const cJSON *rooms,
                     const cJSON *categories,
                     const char *parent_room,
                     const char *parent_category,
                     unsigned depth)
{
    if (!cJSON_IsObject(items) || depth > 8)
        return false;
    const cJSON *control;
    cJSON_ArrayForEach(control, items)
    {
        if (!cJSON_IsObject(control))
            return false;
        const char *room = group_name(rooms, sm_json_string(control, "room"));
        const char *category = group_name(categories, sm_json_string(control, "cat"));
        if (!*room)
            room = parent_room;
        if (!*category)
            category = parent_category;
        const cJSON *states = cJSON_GetObjectItemCaseSensitive(control, "states"), *state;
        if (states && !cJSON_IsObject(states))
            return false;
        cJSON_ArrayForEach(state, states)
        {
            if (!cJSON_IsString(state) || !sm_uuid_valid(state->valuestring))
                continue;
            if (cJSON_GetArraySize(out) >= SM_LX_CATALOG_MAX)
                return false;
            char uuid[SM_UUID_SIZE];
            snprintf(uuid, sizeof(uuid), "%s", state->valuestring);
            for (char *p = uuid; *p; p++)
                *p = tolower((unsigned char)*p);
            cJSON *entry = cJSON_CreateObject();
            bool ok = entry && cJSON_AddStringToObject(entry, "uuid", uuid) &&
                      cJSON_AddStringToObject(entry, "name", sm_json_string(control, "name")) &&
                      cJSON_AddStringToObject(entry, "type", sm_json_string(control, "type")) &&
                      cJSON_AddStringToObject(entry, "state", state->string ? state->string : "") &&
                      cJSON_AddStringToObject(entry, "room", room) &&
                      cJSON_AddStringToObject(entry, "category", category);
            if (!ok || !cJSON_AddItemToArray(out, entry))
            {
                cJSON_Delete(entry);
                return false;
            }
        }
        const cJSON *sub = cJSON_GetObjectItemCaseSensitive(control, "subControls");
        if (sub && !controls(out, sub, rooms, categories, room, category, depth + 1))
            return false;
    }
    return true;
}

cJSON *sm_lx_catalog(const char *json, size_t length)
{
    if (!json || !length || length > SM_LX_STRUCTURE_BYTES)
        return NULL;
    cJSON *root = sm_json_parse(json, length), *out = cJSON_CreateArray();
    bool ok = root && out &&
              controls(out,
                       cJSON_GetObjectItemCaseSensitive(root, "controls"),
                       cJSON_GetObjectItemCaseSensitive(root, "rooms"),
                       cJSON_GetObjectItemCaseSensitive(root, "cats"),
                       "",
                       "",
                       0);
    cJSON_Delete(root);
    if (!ok)
    {
        cJSON_Delete(out);
        return NULL;
    }
    return out;
}
