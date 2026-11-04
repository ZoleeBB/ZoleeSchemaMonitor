#include "app_scene.h"
#include "app_store.h"
#include "bsp/esp-bsp.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "mbedtls/sha256.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

LV_FONT_DECLARE(app_font_20);
typedef struct
{
    lv_image_dsc_t image;
    unsigned char *bytes;
} picture_t;
static picture_t s_pictures[SM_MAX_ELEMENTS];
static lv_point_precise_t s_points[SM_MAX_ELEMENTS][SM_MAX_POINTS];
static lv_obj_t *s_root, *s_toggle;
static atomic_int s_page, s_count;
static atomic_bool s_diagnostics;
static uint32_t s_revision = UINT32_MAX;
static int s_drawn_page = -1;

static void navigate(lv_event_t *event)
{
    int count = atomic_load(&s_count);
    if (!count)
        return;
    int direction = (int)(intptr_t)lv_event_get_user_data(event);
    int page = atomic_load(&s_page);
    atomic_store(&s_page, (page + direction + count) % count);
}

static void toggle(lv_event_t *event)
{
    (void)event;
    bool diagnostics = !atomic_load(&s_diagnostics);
    atomic_store(&s_diagnostics, diagnostics);
    if (s_root)
    {
        if (diagnostics)
            lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);
        else
            lv_obj_remove_flag(s_root, LV_OBJ_FLAG_HIDDEN);
    }
}

static void gesture(lv_event_t *event)
{
    (void)event;
    lv_indev_t *input = lv_indev_active();
    if (!input)
        return;
    int direction = lv_indev_get_gesture_dir(input);
    int count = atomic_load(&s_count), page = atomic_load(&s_page);
    if (count && (direction == LV_DIR_LEFT || direction == LV_DIR_RIGHT))
        atomic_store(&s_page, (page + (direction == LV_DIR_LEFT ? 1 : count - 1)) % count);
}

static cJSON *read_json(const char *path)
{
    FILE *file = fopen(path, "rb");
    if (!file)
        return NULL;
    if (fseek(file, 0, SEEK_END))
    {
        fclose(file);
        return NULL;
    }
    long length = ftell(file);
    if (length <= 0 || length > 262144 || fseek(file, 0, SEEK_SET))
    {
        fclose(file);
        return NULL;
    }
    char *data = malloc(length);
    cJSON *json =
        data && fread(data, 1, length, file) == (size_t)length ? sm_json_parse(data, length) : NULL;
    free(data);
    fclose(file);
    return json;
}

/* Csak katalógusból képzett útvonalat, manifest szerinti méretet és SHA-t fogadunk.
 * Minden fájlolvasás a kijelzőzár felvétele ELŐTT történik. */
static picture_t load_picture(const cJSON *manifest, const char *asset)
{
    picture_t result = {0};
    if (!sm_asset_find(asset) || sm_json_number(manifest, "assetAbi", 0) != 1)
        return result;
    char relative[96], path[128];
    snprintf(relative, sizeof(relative), "light/%s.png", asset);
    const cJSON *entry, *found = NULL;
    cJSON_ArrayForEach(entry,
                       cJSON_GetObjectItemCaseSensitive(
                           manifest, "files")) if (!strcmp(sm_json_string(entry, "path"), relative))
    {
        found = entry;
        break;
    }
    double size = sm_json_number(found, "size", 0);
    if (size < 24 || size > 262144 || size != (size_t)size)
        return result;
    snprintf(path, sizeof(path), "/sdcard/graphics/%s", relative);
    FILE *file = fopen(path, "rb");
    if (!file)
        return result;
    unsigned char *bytes =
        heap_caps_aligned_alloc(128, (size_t)size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    bool ok = bytes && fread(bytes, 1, (size_t)size, file) == (size_t)size && fgetc(file) == EOF;
    fclose(file);
    unsigned char hash[32];
    char digest[65];
    if (ok)
        ok = mbedtls_sha256(bytes, (size_t)size, hash, 0) == 0;
    if (ok)
    {
        for (unsigned i = 0; i < 32; i++)
            snprintf(digest + i * 2, 3, "%02x", hash[i]);
        ok = !strcmp(digest, sm_json_string(found, "sha256")) &&
             !memcmp(bytes, "\211PNG\r\n\032\n", 8) &&
             !memcmp(bytes + 16, "\0\0\0\240\0\0\0\240", 8);
    }
    if (!ok)
    {
        free(bytes);
        return result;
    }
    result.bytes = bytes;
    result.image = (lv_image_dsc_t){.header = {.magic = LV_IMAGE_HEADER_MAGIC,
                                               .cf = LV_COLOR_FORMAT_RAW_ALPHA,
                                               .w = 160,
                                               .h = 160},
                                    .data_size = (uint32_t)size,
                                    .data = bytes};
    return result;
}

static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, lv_color_t color)
{
    lv_obj_t *object = lv_label_create(parent);
    lv_obj_set_style_text_font(object, &app_font_20, 0);
    lv_obj_set_style_text_color(object, color, 0);
    lv_label_set_text(object, text);
    lv_obj_set_pos(object, x, y);
    lv_obj_remove_flag(object, LV_OBJ_FLAG_CLICKABLE);
    return object;
}

static void button(lv_obj_t *parent, const char *text, int x, intptr_t direction)
{
    lv_obj_t *object = lv_button_create(parent);
    lv_obj_set_pos(object, x, 4);
    lv_obj_set_size(object, 48, 36);
    lv_obj_add_event_cb(object, navigate, LV_EVENT_CLICKED, (void *)direction);
    lv_obj_center(label(object, text, 0, 0, lv_color_white()));
}

void app_scene_poll(void)
{
    if (app_store_revision() == s_revision && atomic_load(&s_page) == s_drawn_page)
        return;
    uint32_t revision;
    cJSON *project = app_store_project(&revision);
    if (!project)
        return;
    const cJSON *pages = cJSON_GetObjectItemCaseSensitive(project, "pages");
    int count = cJSON_GetArraySize(pages), page = atomic_load(&s_page);
    atomic_store(&s_count, count);
    if (page >= count)
    {
        page = 0;
        atomic_store(&s_page, 0);
    }
    if (revision == s_revision && page == s_drawn_page)
    {
        cJSON_Delete(project);
        return;
    }
    const cJSON *selected = cJSON_GetArrayItem(pages, page);
    const cJSON *elements = cJSON_GetObjectItemCaseSensitive(selected, "elements");
    picture_t *pictures = calloc(SM_MAX_ELEMENTS, sizeof(*pictures));
    if (!pictures)
    {
        cJSON_Delete(project);
        return;
    }
    cJSON *manifest = read_json("/sdcard/graphics/manifest.json");
    const cJSON *element;
    unsigned index = 0;
    cJSON_ArrayForEach(element, elements)
    {
        if (!strcmp(sm_json_string(element, "type"), "equipment"))
            pictures[index] = load_picture(manifest, sm_json_string(element, "asset"));
        index++;
    }
    cJSON_Delete(manifest);
    if (!bsp_display_lock(1000))
    {
        for (unsigned i = 0; i < SM_MAX_ELEMENTS; i++)
            free(pictures[i].bytes);
        free(pictures);
        cJSON_Delete(project);
        return;
    }
    if (s_root)
        lv_obj_delete(s_root);
    for (unsigned i = 0; i < SM_MAX_ELEMENTS; i++)
    {
        if (s_pictures[i].bytes)
            lv_image_cache_drop(&s_pictures[i].image);
        free(s_pictures[i].bytes);
    }
    memcpy(s_pictures, pictures, sizeof(s_pictures));
    free(pictures);
    s_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(s_root, lv_color_hex(0xf1f5f7), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_root, gesture, LV_EVENT_GESTURE, NULL);
    lv_obj_update_layout(s_root);
    int width = lv_obj_get_width(s_root), height = lv_obj_get_height(s_root);
    index = 0;
    cJSON_ArrayForEach(element, elements)
    {
        const char *type = sm_json_string(element, "type");
        int x = sm_json_number(element, "x", 0) * width,
            y = sm_json_number(element, "y", 0) * height;
        int w = sm_json_number(element, "w", .1) * width,
            h = sm_json_number(element, "h", .1) * height;
        lv_color_t color = lv_color_hex(strtoul(sm_json_string(element, "color") + 1, NULL, 16));
        if (!strcmp(type, "pipe"))
        {
            unsigned n = 0;
            const cJSON *point;
            cJSON_ArrayForEach(point, cJSON_GetObjectItemCaseSensitive(element, "points"))
            {
                s_points[index][n++] = (lv_point_precise_t){sm_json_number(point, "x", 0) * width,
                                                            sm_json_number(point, "y", 0) * height};
            }
            lv_obj_t *line = lv_line_create(s_root);
            lv_line_set_points(line, s_points[index], n);
            lv_obj_set_style_line_color(line, color, 0);
            lv_obj_set_style_line_width(line, sm_json_number(element, "width", 6), 0);
            lv_obj_set_style_line_rounded(line, true, 0);
        }
        else if (!strcmp(type, "equipment"))
        {
            int side = w < h ? w : h;
            if (s_pictures[index].bytes)
            {
                lv_obj_t *image = lv_image_create(s_root);
                lv_image_set_src(image, &s_pictures[index].image);
                lv_image_set_scale(image, side * 256 / 160);
                lv_image_set_pivot(image, 80, 80);
                lv_image_set_rotation(image, sm_json_number(element, "rotation", 0) * 10);
                lv_obj_set_pos(image, x + (w - 160) / 2, y + (h - 160) / 2);
            }
            else
            {
                const sm_asset_t *asset = sm_asset_find(sm_json_string(element, "asset"));
                label(s_root, asset ? asset->name_hu : "?", x, y, color);
            }
            label(s_root, sm_json_string(element, "label"), x, y + h, lv_color_hex(0x345060));
        }
        else
        {
            char text[1200];
            snprintf(text,
                     sizeof(text),
                     "%s%s",
                     !strcmp(type, "value") ? "— " : sm_json_string(element, "text"),
                     !strcmp(type, "value") ? sm_json_string(element, "unit") : "");
            lv_obj_t *object = label(s_root, text, x, y, color);
            lv_obj_set_width(object, w);
            lv_obj_set_style_transform_pivot_x(object, 0, 0);
            lv_obj_set_style_transform_pivot_y(object, 0, 0);
            lv_obj_set_style_transform_scale(
                object, sm_json_number(element, "fontSize", 20) * 256 / 20, 0);
            lv_obj_set_style_transform_rotation(
                object, sm_json_number(element, "rotation", 0) * 10, 0);
        }
        index++;
    }
    button(s_root, "<", 4, -1);
    button(s_root, ">", 60, 1);
    if (!s_toggle)
    {
        s_toggle = lv_button_create(lv_layer_top());
        lv_obj_set_size(s_toggle, 150, 36);
        lv_obj_align(s_toggle, LV_ALIGN_TOP_RIGHT, -4, 4);
        lv_obj_add_event_cb(s_toggle, toggle, LV_EVENT_CLICKED, NULL);
        lv_obj_center(label(s_toggle, "Info / Séma", 0, 0, lv_color_white()));
    }
    if (s_revision == UINT32_MAX && count == 1 && cJSON_GetArraySize(elements) == 0)
        atomic_store(&s_diagnostics, true);
    if (atomic_load(&s_diagnostics))
        lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);
    s_revision = revision;
    s_drawn_page = page;
    bsp_display_unlock();
    cJSON_Delete(project);
}
