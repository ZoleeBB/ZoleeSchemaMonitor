#include "app_scene.h"
#include "app_assets.h"
#include "app_store.h"
#include "bsp/esp-bsp.h"
#include "esp_heap_caps.h"
#include "lvgl.h"
#include "src/misc/cache/instance/lv_image_cache.h"
#include "mbedtls/sha256.h"
#include <stdatomic.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_attr.h"
#define SCENE_BSS EXT_RAM_BSS_ATTR
#else
#define SCENE_BSS
#endif

LV_FONT_DECLARE(app_font_20);
/* A szerkesztő is 55%-os fedettséggel jelzi az ismeretlen aktivitást. */
#define UNKNOWN_OPACITY ((lv_opa_t)140)
typedef struct
{
    lv_image_dsc_t image;
    unsigned char *bytes;
    unsigned owner;
    char digest[65];
} picture_t;
static SCENE_BSS picture_t s_pictures[SM_MAX_ELEMENTS];
static SCENE_BSS lv_point_precise_t s_points[SM_MAX_ELEMENTS][SM_MAX_POINTS];
static lv_obj_t *s_root;
static atomic_bool s_diagnostics;
typedef struct
{
    sm_binding_t value, activity, power, position;
    double position_minimum, position_maximum, position_x, position_y, position_radius, position_rotation;
    bool position_reverse;
    lv_obj_t *position_needle, *position_label;
    lv_point_precise_t position_points[2];
    char last_position[16];
    int last_position_angle;
    double minimum, maximum, phase, length;
    unsigned point_count, decimals, dot_radius;
    int direction;
    lv_obj_t *value_label, *power_label, *body, *pipe_outline, *dots[3], *indicator;
    char unit[100], last_value[160], last_power[64];
    int last_opacity, last_indicator_color, last_indicator_opacity;
} runtime_t;
static SCENE_BSS runtime_t s_runtime[SM_MAX_ELEMENTS];
/* Körönkénti állapot PSRAM-ban; az animáció nem foglal memóriát. */
typedef struct
{
    sm_binding_t source;
    double manual;
    sm_signal_t signal;
    lv_obj_t *indicator, *label;
    int last_color;
    char text[16];
} head_runtime_t;
static SCENE_BSS head_runtime_t s_heads[SM_MAX_ELEMENTS * 16];
static unsigned s_head_count;
typedef struct
{
    unsigned owner;
    lv_obj_t *object;
    double cx, cy, xx, xy, yx, yy;
    double phase, cosine, sine;
    lv_opa_t opacity;
} rotor_t;
static SCENE_BSS rotor_t s_rotors[SM_MAX_ELEMENTS * 8];
static unsigned s_rotor_count;
typedef struct
{
    unsigned owner;
    lv_obj_t *drops[SM_SPRAY_DROPS];
    double x, y, radius, rotation, direction, half;
    double cosine, sine;
    sm_spray_ray_t rays[SM_SPRAY_DROPS];
    bool visible;
} spray_runtime_t;
static SCENE_BSS spray_runtime_t s_sprays[SM_MAX_ELEMENTS];
static unsigned s_spray_count;
static unsigned s_runtime_count;
static app_scene_read_fn s_reader;
static void *s_reader_context;
static sm_binding_t s_night_source;
static bool s_dark, s_drawn_dark, s_english, s_appearance_dirty = true;
static int s_day_brightness = 85, s_night_brightness = 15, s_applied_brightness = -1;

void app_scene_configure(const cJSON *settings)
{
    const cJSON *display = cJSON_GetObjectItemCaseSensitive(settings, "display");
    sm_binding_parse(cJSON_GetObjectItemCaseSensitive(display, "nightSource"), &s_night_source);
    s_day_brightness = sm_json_number(display, "dayBrightness", 85);
    s_night_brightness = sm_json_number(display, "nightBrightness", 15);
    s_english = !strcmp(
        sm_json_string(cJSON_GetObjectItemCaseSensitive(settings, "project"), "language"), "en");
    s_appearance_dirty = true;
}

void app_scene_set_reader(app_scene_read_fn reader, void *context)
{
    s_reader = reader;
    s_reader_context = context;
}

static sm_signal_t signal_read(const sm_binding_t *binding)
{
    double value = 0;
    bool valid = binding->uuid[0] && s_reader && s_reader(binding->uuid, &value, s_reader_context);
    return sm_binding_read(binding, valid, value);
}

static void dot_position(runtime_t *runtime, unsigned index, lv_obj_t *dot, double distance)
{
    for (unsigned i = 1; i < runtime->point_count; i++)
    {
        double dx = s_points[index][i].x - s_points[index][i - 1].x;
        double dy = s_points[index][i].y - s_points[index][i - 1].y;
        double length = hypot(dx, dy);
        if (length > 0 && (distance <= length || i + 1 == runtime->point_count))
        {
            lv_obj_set_pos(dot,
                           s_points[index][i - 1].x + dx * distance / length - runtime->dot_radius,
                           s_points[index][i - 1].y + dy * distance / length - runtime->dot_radius);
            break;
        }
        distance -= length;
    }
}

void app_scene_update(unsigned elapsed_ms)
{
    /* A következő Loxone-adapter RAM-pillanatképe; nincs IO vagy JSON az animációban. */
    static SCENE_BSS sm_signal_t values[SM_MAX_ELEMENTS], powers[SM_MAX_ELEMENTS];
    static SCENE_BSS sm_activity_t states[SM_MAX_ELEMENTS];
    static SCENE_BSS sm_position_t positions[SM_MAX_ELEMENTS];
    for (unsigned i = 0; i < s_runtime_count; i++)
    {
        runtime_t *runtime = &s_runtime[i];
        positions[i] = sm_position(signal_read(&runtime->position), runtime->position_minimum,
                                   runtime->position_maximum, runtime->position_reverse);
        values[i] = signal_read(&runtime->value);
        powers[i] = signal_read(&runtime->power);
        states[i] = sm_activity(
            signal_read(&runtime->activity), powers[i], runtime->minimum, runtime->maximum);
    }
    for (unsigned i = 0; i < s_head_count; i++)
        s_heads[i].signal =
            s_heads[i].source.uuid[0]
                ? signal_read(&s_heads[i].source)
                : (sm_signal_t){.configured = true, .valid = true, .value = s_heads[i].manual};
    if (!bsp_display_lock(100))
        return;
    if (!s_root || atomic_load(&s_diagnostics))
    {
        bsp_display_unlock();
        return;
    }
    for (unsigned i = 0; i < s_head_count; i++)
    {
        head_runtime_t *head = &s_heads[i];
        double percent = fmax(0, fmin(100, head->signal.value));
        int color = !head->signal.valid ? 0xd9a23a : percent > 0 ? 0x36c678 : 0x26333d;
        if (color != head->last_color)
        {
            lv_obj_set_style_bg_color(head->indicator, lv_color_hex(color), 0);
            head->last_color = color;
        }
        char text[16];
        if (head->signal.valid && percent > 0 && percent < .5)
            snprintf(text, sizeof(text), "<1%%");
        else if (head->signal.valid)
            snprintf(text, sizeof(text), "%.0f%%", floor(percent + .5));
        else
            snprintf(text, sizeof(text), "—");
        if (strcmp(text, head->text))
        {
            snprintf(head->text, sizeof(head->text), "%s", text);
            lv_label_set_text_static(head->label, head->text);
        }
    }
    double seconds = (elapsed_ms > 200 ? 200 : elapsed_ms) / 1000.0;
    for (unsigned i = 0; i < s_runtime_count; i++)
    {
        runtime_t *runtime = &s_runtime[i];
        sm_activity_t state = states[i];
        if (runtime->value_label)
        {
            char text[160];
            sm_value_text(values[i], runtime->decimals, runtime->unit, text, sizeof(text));
            if (strcmp(text, runtime->last_value))
            {
                snprintf(runtime->last_value, sizeof(runtime->last_value), "%s", text);
                lv_label_set_text_static(runtime->value_label, runtime->last_value);
            }
        }
        if (runtime->position_label)
        {
            sm_position_t position = positions[i];
            char text[16];
            if (position.known)
                snprintf(text, sizeof(text), "%.0f%%", floor(position.percent + .5));
            else
                snprintf(text, sizeof(text), "—");
            if (strcmp(text, runtime->last_position))
            {
                snprintf(runtime->last_position, sizeof(runtime->last_position), "%s", text);
                lv_label_set_text_static(runtime->position_label, runtime->last_position);
            }
            int angle = position.known ? (int)lround(position.angle * 10) : 9999;
            if (runtime->last_position_angle != angle)
            {
                if (!position.known)
                    lv_obj_add_flag(runtime->position_needle, LV_OBJ_FLAG_HIDDEN);
                else
                {
                    /* A régi és az új mutató területét is újrarajzoljuk. */
                    lv_obj_invalidate(runtime->position_needle);
                    double radians = (angle / 10.0 + runtime->position_rotation - 90) * 3.141592653589793 / 180;
                    for (unsigned n = 0; n < 2; n++)
                    {
                        double radius = runtime->position_radius * (n ? 1 : .2);
                        runtime->position_points[n] = (lv_point_precise_t){
                            lround(runtime->position_x + cos(radians) * radius),
                            lround(runtime->position_y + sin(radians) * radius)};
                    }
                    lv_line_set_points(runtime->position_needle, runtime->position_points, 2);
                    lv_obj_remove_flag(runtime->position_needle, LV_OBJ_FLAG_HIDDEN);
                }
                runtime->last_position_angle = angle;
            }
        }
        if (runtime->power_label)
        {
            char text[64];
            sm_position_t power = sm_position(powers[i], runtime->minimum, runtime->maximum, false);
            sm_value_text((sm_signal_t){true, power.known, power.percent}, 0, "%", text, sizeof(text));
            if (strcmp(text, runtime->last_power))
            {
                snprintf(runtime->last_power, sizeof(runtime->last_power), "%s", text);
                lv_label_set_text_static(runtime->power_label, runtime->last_power);
            }
        }
        int opacity = state.known ? LV_OPA_COVER : UNKNOWN_OPACITY;
        if (runtime->last_opacity != opacity)
        {
            if (runtime->body)
                lv_obj_set_style_opa(runtime->body, opacity, 0);
            if (runtime->pipe_outline)
                lv_obj_set_style_opa(runtime->pipe_outline, opacity, 0);
            if (runtime->position_needle)
            {
                lv_obj_set_style_opa(runtime->position_needle, opacity, 0);
                lv_obj_set_style_opa(runtime->position_label, opacity, 0);
            }
            runtime->last_opacity = opacity;
        }
        if (state.active)
            runtime->phase += seconds * state.speed;
        for (unsigned n = 0; n < 3; n++)
            if (runtime->dots[n])
            {
                if (!state.active || runtime->length <= 0)
                    lv_obj_add_flag(runtime->dots[n], LV_OBJ_FLAG_HIDDEN);
                else
                {
                    lv_obj_remove_flag(runtime->dots[n], LV_OBJ_FLAG_HIDDEN);
                    double distance =
                        fmod(runtime->phase * 50 + runtime->length * n / 3, runtime->length);
                    if (runtime->direction < 0)
                        distance = runtime->length - distance;
                    dot_position(runtime, i, runtime->dots[n], distance);
                }
            }
        if (runtime->indicator)
        {
            int color = !state.known ? 0xd9a23a : state.active ? 0x36c678 : 0x596674;
            int opacity = state.known ? LV_OPA_COVER : UNKNOWN_OPACITY;
            if (state.active)
                opacity = 128 + (int)(127 * (sin(runtime->phase * 6.283185307179586) + 1) / 2);
            if (runtime->last_indicator_color != color)
            {
                lv_obj_set_style_bg_color(runtime->indicator, lv_color_hex(color), 0);
                runtime->last_indicator_color = color;
            }
            if (runtime->last_indicator_opacity != opacity)
            {
                lv_obj_set_style_opa(runtime->indicator, opacity, 0);
                runtime->last_indicator_opacity = opacity;
            }
        }
    }
    for (unsigned n = 0; n < s_spray_count; n++)
    {
        spray_runtime_t *spray = &s_sprays[n];
        bool active = states[spray->owner].active;
        for (unsigned drop = 0; drop < SM_SPRAY_DROPS; drop++)
        {
            lv_obj_t *object = spray->drops[drop];
            if (active != spray->visible)
            {
                if (active)
                    lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN);
                else
                    lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
            }
            if (!active)
                continue;
            double dx, dy;
            if (!sm_spray_sample(s_runtime[spray->owner].phase, drop, &spray->rays[drop], &dx, &dy))
                continue;
            double rx = (dx * spray->cosine - dy * spray->sine) * spray->radius;
            double ry = (dx * spray->sine + dy * spray->cosine) * spray->radius;
            lv_obj_set_pos(object, lround(spray->x + rx - spray->half), lround(spray->y + ry - spray->half));
        }
        spray->visible = active;
    }
    for (unsigned n = 0; n < s_rotor_count; n++)
    {
        rotor_t *rotor = &s_rotors[n];
        double phase = s_runtime[rotor->owner].phase;
        lv_opa_t opacity = states[rotor->owner].known ? LV_OPA_COVER : UNKNOWN_OPACITY;
        if (phase != rotor->phase || opacity != rotor->opacity)
        {
            rotor->phase = phase;
            rotor->cosine = cos(phase * 6.283185307179586);
            rotor->sine = sin(phase * 6.283185307179586);
            if (rotor->opacity != opacity)
                lv_obj_set_style_opa_layered(rotor->object, opacity, 0);
            rotor->opacity = opacity;
            lv_obj_invalidate(rotor->object);
        }
    }
    bsp_display_unlock();
}

static lv_obj_t *s_toggle;
static atomic_int s_page, s_count;
static uint32_t s_revision = UINT32_MAX, s_assets_revision;
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

/* Csak katalógusból képzett útvonalat, manifest szerinti méretet és SHA-t fogadunk.
 * Minden fájlolvasás a kijelzőzár felvétele ELŐTT történik. */
static picture_t load_picture(
    const cJSON *manifest, const char *asset, bool dark, const picture_t *loaded, unsigned count)
{
    picture_t result = {.owner = count};
    unsigned circuits = sm_manifold_raster_circuits(asset);
    unsigned width = circuits ? 96 + 56 * circuits : 160;
    unsigned height = circuits ? 224 : 160;
    if ((!circuits && !sm_asset_find(asset)) || sm_json_number(manifest, "assetAbi", 0) != 1)
        return result;
    char relative[96], path[APP_ASSET_PATH_SIZE];
    snprintf(relative, sizeof(relative), "%s/%s.png", dark ? "dark" : "light", asset);
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
    const char *expected_hash = sm_json_string(found, "sha256");
    if (strlen(expected_hash) != 64)
        return result;
    /* Azonos tartalomhoz egyetlen PNG-puffer és LVGL cache-kulcs tartozik. */
    for (unsigned i = 0; i < count; i++)
        if (loaded[i].bytes && loaded[i].image.header.w == width && loaded[i].image.header.h == height &&
            loaded[i].image.data_size == (size_t)size &&
            !strcmp(loaded[i].digest, expected_hash))
            return loaded[i];
    if (!app_assets_path(manifest, relative, path))
        return result;
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
             !memcmp(bytes + 12, "IHDR", 4) &&
             bytes[16] == 0 && bytes[17] == 0 && bytes[18] == width / 256 &&
             bytes[19] == width % 256 && bytes[20] == 0 && bytes[21] == 0 &&
             bytes[22] == 0 && bytes[23] == height;
    }
    if (!ok)
    {
        free(bytes);
        return result;
    }
    result.bytes = bytes;
    snprintf(result.digest, sizeof(result.digest), "%s", digest);
    result.image = (lv_image_dsc_t){.header = {.magic = LV_IMAGE_HEADER_MAGIC,
                                               .cf = LV_COLOR_FORMAT_RAW_ALPHA,
                                               .w = width,
                                               .h = height},
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

typedef struct
{
    double x, y, w, h, angle;
} manifold_geometry_t;
/* A képet, az élő rétegeket és a csővégeket ugyanabba a keretbe illesztjük. */
static manifold_geometry_t equipment_geometry(const cJSON *element, int x, int y, int w, int h)
{
    const cJSON *m = cJSON_GetObjectItemCaseSensitive(element, "manifold");
    unsigned n = m ? (unsigned)sm_json_number(m, "circuits", 4)
                   : sm_manifold_raster_circuits(sm_json_string(element, "asset"));
    double iw = n ? 96 + 56 * n : 160, ih = n ? 224 : 160;
    double scale = fmin(w / iw, h / ih);
    return (manifold_geometry_t){x + (w - iw * scale) / 2, y + (h - ih * scale) / 2,
                                 iw * scale, ih * scale,
                                 sm_json_number(element, "rotation", 0) * 3.141592653589793 / 180};
}

static void manifold_point(manifold_geometry_t g, double px, double py, double *x, double *y)
{
    double dx = (px - .5) * g.w, dy = (py - .5) * g.h;
    *x = g.x + g.w / 2 + dx * cos(g.angle) - dy * sin(g.angle);
    *y = g.y + g.h / 2 + dx * sin(g.angle) + dy * cos(g.angle);
}
/* A konvex lapát két szélét soronként követjük. Így a kitöltésben
 * élsimításkor sincsenek áttetsző illesztések a háromszögek között. */
static float blade_edge_x(const lv_point_t *polygon, int count, int *edge, int step, int y)
{
    int next = (*edge + step + count) % count;
    while (polygon[next].y <= y)
    {
        *edge = next;
        next = (*edge + step + count) % count;
    }
    lv_point_t a = polygon[*edge], b = polygon[next];
    return a.x + (b.x - a.x) * (float)(y - a.y) / (b.y - a.y);
}

static void fill_blade(lv_layer_t *layer, const lv_draw_fill_dsc_t *fill,
                       const lv_point_t *polygon, int count)
{
    int top = 0, bottom = polygon[0].y;
    for (int i = 1; i < count; i++)
    {
        if (polygon[i].y < polygon[top].y)
            top = i;
        if (polygon[i].y > bottom)
            bottom = polygon[i].y;
    }
    int left = top, right = top;
    for (int y = polygon[top].y; y < bottom; y++)
    {
        float a = blade_edge_x(polygon, count, &left, 1, y);
        float b = blade_edge_x(polygon, count, &right, -1, y);
        lv_area_t span = {lroundf(fminf(a, b)), y, lroundf(fmaxf(a, b)), y};
        lv_draw_fill(layer, fill, &span);
    }
}

/* A szerkesztő két másodfokú Bézier-íve. Először a lapátot forgatjuk,
 * utána vetítjük az ellipszisre, végül a készülékkel együtt fordítjuk el. */
static void draw_rotor(lv_event_t *event)
{
    const rotor_t *rotor = lv_event_get_user_data(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t bounds;
    lv_obj_get_coords(rotor->object, &bounds);
    double cx = bounds.x1 + rotor->cx, cy = bounds.y1 + rotor->cy;
    double c = rotor->cosine, s = rotor->sine;
    enum { steps = 10, points = 2 * steps };
    lv_draw_fill_dsc_t fill;
    lv_draw_fill_dsc_init(&fill);
    fill.color = lv_color_hex(0x9cb0be);
    lv_draw_line_dsc_t stroke;
    lv_draw_line_dsc_init(&stroke);
    stroke.color = lv_color_hex(0xe5eff4);
    stroke.width = 1;
    for (unsigned blade = 0; blade < 3; blade++)
    {
        double xx = rotor->xx * c + rotor->xy * s, xy = rotor->xy * c - rotor->xx * s;
        double yx = rotor->yx * c + rotor->yy * s, yy = rotor->yy * c - rotor->yx * s;
        lv_point_t polygon[points];
        for (int i = 0; i < points; i++)
        {
            double t = (double)(i <= steps ? i : points - i) / steps;
            double x = 2 * t * (1 - t) * (i <= steps ? -1 : 1);
            double y = t * (t - 2);
            polygon[i] = (lv_point_t){lround(cx + xx * x + xy * y), lround(cy + yx * x + yy * y)};
        }
        fill_blade(layer, &fill, polygon, points);
        for (int i = 0; i < points; i++)
        {
            lv_point_t a = polygon[i], b = polygon[(i + 1) % points];
            if (a.x == b.x && a.y == b.y)
                continue;
            stroke.p1 = (lv_point_precise_t){a.x, a.y};
            stroke.p2 = (lv_point_precise_t){b.x, b.y};
            lv_draw_line(layer, &stroke);
        }
        double next_c = -.5 * c - .8660254037844386 * s;
        s = .8660254037844386 * c - .5 * s;
        c = next_c;
    }
}

static void add_rotor(unsigned owner, manifold_geometry_t g, const sm_asset_t *asset,
                      double x, double y, double radius)
{
    double cx, cy;
    manifold_point(g, x, y, &cx, &cy);
    double rx = radius * g.w * asset->rotor_scale_x, ry = radius * g.h * asset->rotor_scale_y;
    double c = cos(g.angle), s = sin(g.angle);
    int half_w = ceil(1.02 * hypot(rx * c, ry * s)) + 2, half_h = ceil(1.02 * hypot(rx * s, ry * c)) + 2;
    int left = lround(cx) - half_w, top = lround(cy) - half_h;
    rotor_t *rotor = &s_rotors[s_rotor_count++];
    *rotor = (rotor_t){.owner = owner, .object = lv_obj_create(s_root),
                       .cx = cx - left, .cy = cy - top,
                       .xx = rx * c, .xy = -ry * s, .yx = rx * s, .yy = ry * c,
                       .cosine = 1, .opacity = LV_OPA_COVER};
    lv_obj_remove_style_all(rotor->object);
    lv_obj_remove_flag(rotor->object, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(rotor->object, left, top);
    lv_obj_set_size(rotor->object, 2 * half_w + 1, 2 * half_h + 1);
    lv_obj_add_event_cb(rotor->object, draw_rotor, LV_EVENT_DRAW_MAIN, rotor);
}

static lv_obj_t *manifold_rect(manifold_geometry_t g, double x, double y, double w, double h,
                               unsigned color)
{
    double cx, cy;
    manifold_point(g, x + w / 2, y + h / 2, &cx, &cy);
    double rw = fabs(cos(g.angle)) * w * g.w + fabs(sin(g.angle)) * h * g.h;
    double rh = fabs(sin(g.angle)) * w * g.w + fabs(cos(g.angle)) * h * g.h;
    lv_obj_t *obj = lv_obj_create(s_root);
    lv_obj_remove_style_all(obj);
    lv_obj_set_size(obj, fmax(1, lround(rw)), fmax(1, lround(rh)));
    lv_obj_set_pos(obj, lround(cx - rw / 2), lround(cy - rh / 2));
    lv_obj_set_style_radius(obj, fmin(3, fmin(rw, rh) * .15), 0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x67747d), 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    return obj;
}
static lv_obj_t *manifold_label(manifold_geometry_t g, double x, double y, const char *text,
                                int font_size)
{
    double cx, cy;
    manifold_point(g, x, y, &cx, &cy);
    lv_obj_t *obj = label(s_root, text, lround(cx - 40), lround(cy),
                          lv_color_hex(s_dark ? 0xdfecf4 : 0x354e60));
    int text_width = strlen(text) > 6 ? 260 : 80;
    lv_obj_set_width(obj, text_width);
    lv_obj_set_pos(obj, lround(cx - text_width / 2), lround(cy));
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_transform_pivot_x(obj, text_width / 2, 0);
    lv_obj_set_style_transform_pivot_y(obj, 0, 0);
    lv_obj_set_style_transform_scale(obj, font_size * 256 / 20, 0);
    return obj;
}
static lv_obj_t *draw_picture(unsigned index, int cx, int cy, double width, int rotation)
{
    const lv_image_dsc_t *src = &s_pictures[s_pictures[index].owner].image;
    int iw = src->header.w, ih = src->header.h;
    lv_obj_t *image = lv_image_create(s_root);
    lv_image_set_src(image, src);
    lv_image_set_pivot(image, iw / 2, ih / 2);
    lv_image_set_scale(image, fmax(1, lround(width * 256.0 / iw)));
    lv_image_set_rotation(image, rotation * 10);
    lv_obj_set_pos(image, cx - iw / 2, cy - ih / 2);
    return image;
}

static void draw_manifold(const cJSON *m, manifold_geometry_t g)
{
    unsigned n = sm_json_number(m, "circuits", 4);
    if (n < 2 || n > 16)
        return;
    bool connections = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(m, "connections"));
    const char *variant = sm_json_string(m, "variant");
    unsigned supply = !strcmp(variant, "ceiling-cool") ? 0x349bd0 : 0xdb5952;
    const char *caption =
        !strcmp(variant, "ceiling-cool")   ? (s_english ? "Ceiling cooling" : "Mennyezethűtés")
        : !strcmp(variant, "ceiling-heat") ? (s_english ? "Ceiling heating" : "Mennyezetfűtés")
        : !strcmp(variant, "floor")        ? (s_english ? "Underfloor heating" : "Padlófűtés")
                                           : (s_english ? "Heating" : "Fűtés");
    double scale = g.h / 224;
    manifold_label(g, .5, .0, caption, fmax(1, lround(12 * scale)));
    double width = 96 + 56 * n;
    if (connections)
    {
        manifold_rect(g, 0, 95./224, 5/width, 8./224, supply);
        manifold_rect(g, 0, 187./224, 5/width, 8./224, 0x349bd0);
    }
    for (unsigned i = 0; i < n; i++)
    {
        double x = (56 + 56 * (i + .5)) / width;
        head_runtime_t *head = &s_heads[s_head_count++];
        const cJSON *json = cJSON_GetArrayItem(cJSON_GetObjectItemCaseSensitive(m, "heads"), i);
        sm_binding_parse(cJSON_GetObjectItemCaseSensitive(json, "source"), &head->source);
        head->manual = sm_json_number(json, "percent", 0);
        head->last_color = -1;
        head->indicator = manifold_rect(g, x - 8/width, 140./224, 16/width, 5./224, 0x26333d);
        int font_size = fmax(1, lround(13 * scale));
        manifold_geometry_t text_box = g;
        text_box.y -= font_size * .6;
        head->label = manifold_label(text_box, x, 155./224, "—", font_size);
        lv_obj_set_style_text_color(head->label, lv_color_hex(0x26333d), 0);
        char number[8];
        snprintf(number, sizeof(number), "%u", i + 1);
        manifold_label(g, x, .065, number, fmax(1, lround(11 * scale)));
        if (connections)
        {
            manifold_rect(g, x - 4/width, 123./224, 8/width, 6./224, supply);
            manifold_rect(g, x - 4/width, 216./224, 8/width, 6./224, 0x349bd0);
        }
    }
}

static void button(lv_obj_t *parent, const char *text, int x, intptr_t direction)
{
    lv_obj_t *object = lv_button_create(parent);
    lv_obj_set_pos(object, x, 4);
    lv_obj_set_size(object, 48, 36);
    lv_obj_add_event_cb(object, navigate, LV_EVENT_CLICKED, (void *)direction);
    lv_obj_center(label(object, text, 0, 0, lv_color_white()));
}

/* A kapcsolt végpontot mindig az aktuális grafika csonkjából számoljuk.
 * Régi mentésben maradt helyőrző-koordináta nem hagyhat rést az új kép mellett. */
static bool equipment_port_position(const cJSON *element, unsigned port, int width, int height,
                                    lv_point_precise_t *result)
{
    const sm_asset_t *asset = sm_asset_find(sm_json_string(element, "asset"));
    if (!asset || strcmp(sm_json_string(element, "type"), "equipment"))
        return false;
    const cJSON *m = cJSON_GetObjectItemCaseSensitive(element, "manifold");
    sm_point_t anchor;
    if (m)
    {
        unsigned count = sm_json_number(m, "circuits", 0);
        if (!cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(m, "connections")) || count < 2 ||
            count > 16 || port >= 2 + 2 * count)
            return false;
        if (port < 2)
            anchor = (sm_point_t){0, (port ? 191. : 99.) / 224};
        else
            anchor = (sm_point_t){(56 + 56 * ((port - 2) / 2 + .5)) / (96 + 56 * count),
                                  (port % 2 ? 222. : 129.) / 224};
    }
    else
    {
        if (port >= asset->port_count)
            return false;
        anchor = asset->ports[port];
    }
    int x = sm_json_number(element, "x", 0) * width;
    int y = sm_json_number(element, "y", 0) * height;
    int w = sm_json_number(element, "w", .1) * width;
    int h = sm_json_number(element, "h", .1) * height;
    double px, py;
    manifold_point(equipment_geometry(element, x, y, w, h), anchor.x, anchor.y, &px, &py);
    *result = (lv_point_precise_t){px, py};
    return true;
}

static void connect_pipe_endpoints(const cJSON *pipe, const cJSON *elements, unsigned index,
                                   unsigned count, int width, int height)
{
    const char *keys[] = {"source", "target"};
    for (unsigned k = 0; k < 2; k++)
    {
        const cJSON *end = cJSON_GetObjectItemCaseSensitive(pipe, keys[k]);
        const char *id = sm_json_string(end, "element");
        if (!id[0])
            continue;
        const cJSON *equipment;
        cJSON_ArrayForEach(equipment, elements) if (!strcmp(sm_json_string(equipment, "id"), id))
        {
            equipment_port_position(equipment, sm_json_number(end, "port", 999), width, height,
                                    &s_points[index][k ? count - 1 : 0]);
            break;
        }
    }
}

void app_scene_poll(void)
{
    /* Ismeretlen éjszakai jel nem egyenlő nappallal: az utolsó ismert mód marad.
     * Induláskor nappali a biztonságos alapérték. A BSP-hívás kijelzőzáron kívül fut. */
    sm_signal_t night = signal_read(&s_night_source);
    if (night.valid)
        s_dark = night.value > 0;
    int brightness = s_dark ? s_night_brightness : s_day_brightness;
    if (brightness != s_applied_brightness && bsp_display_brightness_set(brightness) == ESP_OK)
        s_applied_brightness = brightness;
    uint32_t assets_revision = app_assets_revision();
    bool appearance_changed =
        s_appearance_dirty || s_dark != s_drawn_dark || assets_revision != s_assets_revision;
    if (!appearance_changed && app_store_revision() == s_revision &&
        atomic_load(&s_page) == s_drawn_page)
        return;
    uint32_t revision;
    int page = atomic_load(&s_page), count;
    cJSON *selected = app_store_page(page, &count, &revision);
    if (!selected)
        return;
    atomic_store(&s_count, count);
    if (page >= count)
    {
        int requested = page;
        atomic_compare_exchange_strong(&s_page, &requested, 0);
        page = 0;
    }
    if (!appearance_changed && revision == s_revision && page == s_drawn_page)
    {
        cJSON_Delete(selected);
        return;
    }
    const cJSON *elements = cJSON_GetObjectItemCaseSensitive(selected, "elements");
    picture_t *pictures = calloc(SM_MAX_ELEMENTS, sizeof(*pictures));
    if (!pictures)
    {
        cJSON_Delete(selected);
        return;
    }
    cJSON *manifest = app_assets_manifest();
    const cJSON *element;
    unsigned index = 0;
    cJSON_ArrayForEach(element, elements)
    {
        if (!strcmp(sm_json_string(element, "type"), "equipment"))
        {
            const cJSON *m = cJSON_GetObjectItemCaseSensitive(element, "manifold");
            const char *id = sm_json_string(element, "asset");
            char body[32];
            if (m)
            {
                snprintf(body, sizeof(body), "manifold-%s-%u",
                         !strcmp(sm_json_string(m, "variant"), "ceiling-cool") ? "cool" : "body",
                         (unsigned)sm_json_number(m, "circuits", 4));
                id = body;
            }
            pictures[index] = load_picture(manifest, id, !m && s_dark, pictures, index);
        }
        index++;
    }
    cJSON_Delete(manifest);
    if (!bsp_display_lock(1000))
    {
        for (unsigned i = 0; i < SM_MAX_ELEMENTS; i++)
            if (pictures[i].owner == i)
                free(pictures[i].bytes);
        free(pictures);
        cJSON_Delete(selected);
        return;
    }
    if (s_root)
        lv_obj_delete(s_root);
    for (unsigned i = 0; i < SM_MAX_ELEMENTS; i++)
    {
        if (s_pictures[i].bytes && s_pictures[i].owner == i)
            lv_image_cache_drop(&s_pictures[i].image);
        if (s_pictures[i].owner == i)
            free(s_pictures[i].bytes);
    }
    memcpy(s_pictures, pictures, sizeof(s_pictures));
    free(pictures);
    memset(s_runtime, 0, sizeof(s_runtime));
    memset(s_heads, 0, sizeof(s_heads));
    s_head_count = 0;
    s_rotor_count = 0;
    memset(s_sprays, 0, sizeof(s_sprays));
    s_spray_count = 0;
    s_runtime_count = cJSON_GetArraySize(elements);
    s_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, LV_PCT(100), LV_PCT(100));
    const char *background = sm_json_string(selected, "background");
    bool warm = !strcmp(background, "warm"), plain = !strcmp(background, "plain");
    uint32_t first = s_dark ? 0x17232f : warm ? 0xfaf8f1 : 0xf7fafc;
    uint32_t last = s_dark ? 0x25394a : warm ? 0xe9e1d7 : 0xdfeaf0;
    lv_obj_set_style_bg_color(s_root, lv_color_hex(first), 0);
    lv_obj_set_style_bg_grad_color(s_root, lv_color_hex(last), 0);
    lv_obj_set_style_bg_grad_dir(s_root, plain ? LV_GRAD_DIR_NONE : LV_GRAD_DIR_VER, 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_root, gesture, LV_EVENT_GESTURE, NULL);
    lv_obj_update_layout(s_root);
    int width = lv_obj_get_width(s_root), height = lv_obj_get_height(s_root);
    index = 0;
    cJSON_ArrayForEach(element, elements)
    {
        runtime_t *runtime = &s_runtime[index];
        sm_binding_parse(cJSON_GetObjectItemCaseSensitive(element, "value"), &runtime->value);
        sm_binding_parse(cJSON_GetObjectItemCaseSensitive(element, "activity"), &runtime->activity);
        const cJSON *power = cJSON_GetObjectItemCaseSensitive(element, "power");
        sm_binding_parse(cJSON_GetObjectItemCaseSensitive(power, "source"), &runtime->power);
        runtime->minimum = sm_json_number(power, "minimum", 0);
        runtime->maximum = sm_json_number(power, "maximum", 100);
        const cJSON *position = cJSON_GetObjectItemCaseSensitive(element, "position");
        sm_binding_parse(cJSON_GetObjectItemCaseSensitive(position, "source"), &runtime->position);
        runtime->position_minimum = sm_json_number(position, "minimum", 0);
        runtime->position_maximum = sm_json_number(position, "maximum", 100);
        runtime->position_reverse = cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(position, "reverse"));
        runtime->last_position_angle = 9998;
        runtime->decimals = sm_json_number(element, "decimals", 1);
        runtime->direction = sm_json_number(element, "direction", 1);
        runtime->last_opacity = -1;
        snprintf(runtime->unit, sizeof(runtime->unit), "%s", sm_json_string(element, "unit"));
        const char *type = sm_json_string(element, "type");
        int x = sm_json_number(element, "x", 0) * width,
            y = sm_json_number(element, "y", 0) * height;
        int w = sm_json_number(element, "w", .1) * width,
            h = sm_json_number(element, "h", .1) * height;
        unsigned rgb = strtoul(sm_json_string(element, "color") + 1, NULL, 16);
        lv_color_t color = lv_color_hex(rgb);
        if (s_dark && strcmp(type, "pipe") && !strcmp(sm_json_string(element, "color"), "#456172"))
            color = lv_color_hex(0xd5e6f0);
        if (!strcmp(type, "pipe"))
        {
            unsigned n = 0;
            const cJSON *point;
            cJSON_ArrayForEach(point, cJSON_GetObjectItemCaseSensitive(element, "points"))
            {
                s_points[index][n++] = (lv_point_precise_t){sm_json_number(point, "x", 0) * width,
                                                            sm_json_number(point, "y", 0) * height};
            }
            if (n >= 2)
                connect_pipe_endpoints(element, elements, index, n, width, height);
            runtime->point_count = n;
            for (unsigned k = 1; k < n; k++)
                runtime->length += hypot(s_points[index][k].x - s_points[index][k - 1].x,
                                         s_points[index][k].y - s_points[index][k - 1].y);
            int line_width = sm_json_number(element, "width", 6);
            runtime->dot_radius = fmax(2, lround(line_width * .3));
            unsigned luminance = 299 * (rgb >> 16) + 587 * ((rgb >> 8) & 255) + 114 * (rgb & 255);
            lv_color_t dot_color = lv_color_hex(luminance >= 145000 ? 0x15202b : 0xffffff);
            if (runtime->activity.uuid[0] || runtime->power.uuid[0])
                for (unsigned k = 0; k < 3; k++)
                {
                    lv_obj_t *dot = lv_obj_create(s_root);
                    lv_obj_remove_style_all(dot);
                    lv_obj_set_size(dot, 2 * runtime->dot_radius, 2 * runtime->dot_radius);
                    lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
                    lv_obj_set_style_bg_color(dot, dot_color, 0);
                    if (line_width < 4)
                    {
                        lv_obj_set_style_border_color(dot, color, 0);
                        lv_obj_set_style_border_width(dot, 1, 0);
                    }
                    lv_obj_set_style_bg_opa(dot, LV_OPA_COVER, 0);
                    lv_obj_add_flag(dot, LV_OBJ_FLAG_HIDDEN);
                    runtime->dots[k] = dot;
                }
            if (s_dark ? luminance < 70000 : luminance > 205000)
            {
                lv_obj_t *outline = lv_line_create(s_root);
                lv_line_set_points(outline, s_points[index], n);
                lv_obj_set_style_line_color(outline, lv_color_hex(s_dark ? 0x9baab5 : 0x738391), 0);
                lv_obj_set_style_line_width(outline, line_width + 2, 0);
                lv_obj_set_style_line_rounded(outline, true, 0);
                runtime->pipe_outline = outline;
            }
            lv_obj_t *line = lv_line_create(s_root);
            runtime->body = line;
            lv_line_set_points(line, s_points[index], n);
            lv_obj_set_style_line_color(line, color, 0);
            lv_obj_set_style_line_width(line, line_width, 0);
            lv_obj_set_style_line_rounded(line, true, 0);
            for (unsigned k = 0; k < 3; k++)
                if (runtime->dots[k])
                    lv_obj_move_foreground(runtime->dots[k]);
        }
        else if (!strcmp(type, "equipment"))
        {
            int side = w < h ? w : h;
            manifold_geometry_t geometry = equipment_geometry(element, x, y, w, h);
            const cJSON *manifold = cJSON_GetObjectItemCaseSensitive(element, "manifold");
            if (s_pictures[index].bytes)
            {
                lv_obj_t *image = draw_picture(index, x + w / 2, y + h / 2,
                                               geometry.w,
                                               sm_json_number(element, "rotation", 0));
                if (!manifold)
                    runtime->body = image;
            }
            if (manifold)
                draw_manifold(manifold, geometry);
            else if (!s_pictures[index].bytes)
            {
                const sm_asset_t *asset = sm_asset_find(sm_json_string(element, "asset"));
                label(s_root,
                      asset ? (s_english ? asset->name_en : asset->name_hu) : "?",
                      x,
                      y,
                      color);
            }
            label(s_root,
                  sm_json_string(element, "label"),
                  x,
                  y + h,
                  lv_color_hex(s_dark ? 0xdfecf4 : 0x354e60));
            if (cJSON_IsTrue(cJSON_GetObjectItemCaseSensitive(element, "showPower")))
                runtime->power_label = label(s_root, "— %", x, y + h + 24, color);
            const sm_asset_t *asset = sm_asset_find(sm_json_string(element, "asset"));
            double marker_x = 0, marker_y = 0;
            if (asset && asset->animation)
            {
                if (runtime->body)
                {
                    const lv_image_dsc_t *image = &s_pictures[s_pictures[index].owner].image;
                    double scale = lv_image_get_scale(runtime->body) / 256.0;
                    geometry.w = image->header.w * scale;
                    geometry.h = image->header.h * scale;
                    geometry.x = x + w / 2 - (image->header.w / 2) * scale;
                    geometry.y = y + h / 2 - (image->header.h / 2) * scale;
                }
                manifold_point(geometry, asset->animation_x, asset->animation_y, &marker_x, &marker_y);
            }
            if (asset && (asset->animation == 3 || asset->animation == 2))
            {
                int radius = lround(fmax(2, (asset->animation == 2 ? .025 : asset->animation_radius) * geometry.w));
                runtime->indicator = lv_obj_create(s_root);
                lv_obj_remove_style_all(runtime->indicator);
                lv_obj_set_size(runtime->indicator, radius * 2, radius * 2);
                lv_obj_set_pos(runtime->indicator,
                               lround(marker_x) - radius, lround(marker_y) - radius);
                lv_obj_set_style_radius(runtime->indicator, LV_RADIUS_CIRCLE, 0);
                lv_obj_set_style_bg_opa(runtime->indicator, LV_OPA_COVER, 0);
                lv_obj_set_style_border_color(runtime->indicator, lv_color_hex(0xe5eff4), 0);
                lv_obj_set_style_border_width(runtime->indicator, 1, 0);
            }
            if (asset && asset->animation == 2)
            {
                spray_runtime_t *spray = &s_sprays[s_spray_count++];
                spray->owner = index;
                spray->rotation = geometry.angle;
                spray->cosine = cos(spray->rotation);
                spray->sine = sin(spray->rotation);
                spray->x = marker_x;
                spray->y = marker_y;
                spray->radius = asset->animation_radius * geometry.w;
                spray->direction = asset->spray_direction;
                int diameter = lround(fmax(3, geometry.w * .025));
                spray->half = diameter / 2.0;
                for (unsigned drop = 0; drop < SM_SPRAY_DROPS; drop++)
                {
                    sm_spray_prepare(drop, spray->direction, &spray->rays[drop]);
                    lv_obj_t *object = lv_obj_create(s_root);
                    spray->drops[drop] = object;
                    lv_obj_remove_style_all(object);
                    lv_obj_set_size(object, diameter, diameter);
                    lv_obj_set_style_radius(object, LV_RADIUS_CIRCLE, 0);
                    lv_obj_set_style_bg_color(object, lv_color_hex(0x249bd0), 0);
                    lv_obj_set_style_bg_opa(object, LV_OPA_COVER, 0);
                    lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
                    lv_obj_remove_flag(object, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE);
                }
            }
            if (asset && asset->animation == 4 && runtime->position.uuid[0] && s_pictures[index].bytes)
            {
                runtime->position_rotation = sm_json_number(element, "rotation", 0);
                runtime->position_radius = asset->animation_radius * geometry.w;
                int line_width = lround(fmax(2, geometry.w * .018));
                int half = ceil(runtime->position_radius + line_width * .5) + 1;
                int left = lround(marker_x) - half, top = lround(marker_y) - half;
                runtime->position_x = marker_x - left;
                runtime->position_y = marker_y - top;
                runtime->position_needle = lv_line_create(s_root);
                lv_obj_set_pos(runtime->position_needle, left, top);
                lv_obj_set_size(runtime->position_needle, 2 * half + 1, 2 * half + 1);
                lv_line_set_points(runtime->position_needle, runtime->position_points, 2);
                lv_obj_set_style_line_color(runtime->position_needle, lv_color_hex(0xff3b30), 0);
                lv_obj_set_style_line_width(runtime->position_needle, line_width, 0);
                lv_obj_set_style_line_rounded(runtime->position_needle, true, 0);
                lv_obj_add_flag(runtime->position_needle, LV_OBJ_FLAG_HIDDEN);
                /* A tördelés még a méretezés előtt történik: legyen hely a 100% feliratnak. */
                int text_scale = fmax(8, side * .065) * 256 / 20;
                int text_width = fmax(80, ceil(side * .24 * 256 / text_scale));
                runtime->position_label = label(s_root, "—", lround(marker_x - text_width / 2.0),
                                              lround(marker_y + side * .025), lv_color_white());
                lv_obj_set_width(runtime->position_label, text_width);
                lv_obj_set_style_text_align(runtime->position_label, LV_TEXT_ALIGN_CENTER, 0);
                lv_obj_set_style_transform_pivot_x(runtime->position_label, text_width / 2, 0);
                lv_obj_set_style_transform_pivot_y(runtime->position_label, 0, 0);
                lv_obj_set_style_transform_scale(runtime->position_label, text_scale, 0);
            }
            if (asset && asset->animation == 1)
            {
                add_rotor(index, geometry, asset, asset->animation_x, asset->animation_y, asset->animation_radius);
                for (unsigned n = 0; n < asset->extra_rotor_count; n++)
                    add_rotor(index, geometry, asset, asset->extra_rotors[n].x,
                              asset->extra_rotors[n].y, asset->extra_rotors[n].radius);
            }
        }
        else
        {
            bool is_value = !strcmp(type, "value");
            const char *text = sm_json_string(element, "text");
            if (is_value)
            {
                sm_value_text((sm_signal_t){0}, runtime->decimals, runtime->unit,
                              runtime->last_value, sizeof(runtime->last_value));
                text = runtime->last_value;
            }
            lv_obj_t *container = lv_obj_create(s_root);
            lv_obj_remove_style_all(container);
            lv_obj_set_pos(container, x, y);
            lv_obj_set_size(container, w, h);
            lv_obj_add_flag(container, LV_OBJ_FLAG_OVERFLOW_VISIBLE);
            lv_obj_set_style_transform_pivot_x(container, w / 2, 0);
            lv_obj_set_style_transform_pivot_y(container, h / 2, 0);
            lv_obj_set_style_transform_rotation(container,
                                                sm_json_number(element, "rotation", 0) * 10, 0);
            lv_obj_t *object = label(container, text, 0, 0, color);
            lv_obj_set_style_text_line_space(object, 0, 0);
            lv_obj_set_style_text_letter_space(object, 0, 0);
            if (is_value)
                runtime->value_label = object;
            lv_obj_set_width(object, w * 20 / sm_json_number(element, "fontSize", 20));
            const char *alignment = sm_json_string(element, "align");
            lv_text_align_t text_alignment = LV_TEXT_ALIGN_LEFT;
            if (!strcmp(alignment, "center"))
                text_alignment = LV_TEXT_ALIGN_CENTER;
            else if (!strcmp(alignment, "right"))
                text_alignment = LV_TEXT_ALIGN_RIGHT;
            lv_obj_set_style_text_align(object, text_alignment, 0);
            lv_obj_set_style_transform_pivot_x(object, 0, 0);
            lv_obj_set_style_transform_pivot_y(object, 0, 0);
            lv_obj_set_style_transform_scale(
                object, sm_json_number(element, "fontSize", 20) * 256 / 20, 0);
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
        lv_obj_center(
            label(s_toggle, s_english ? "Info / Schema" : "Info / Séma", 0, 0, lv_color_white()));
    }
    if (s_revision == UINT32_MAX && count == 1 && cJSON_GetArraySize(elements) == 0)
        atomic_store(&s_diagnostics, true);
    if (atomic_load(&s_diagnostics))
        lv_obj_add_flag(s_root, LV_OBJ_FLAG_HIDDEN);
    if (s_appearance_dirty)
        lv_label_set_text(lv_obj_get_child(s_toggle, 0),
                          s_english ? "Info / Schema" : "Info / Séma");
    s_drawn_dark = s_dark;
    s_appearance_dirty = false;
    s_assets_revision = assets_revision;
    s_revision = revision;
    s_drawn_page = page;
    bsp_display_unlock();
    cJSON_Delete(selected);
}

bool app_scene_diagnostics_visible(void)
{
    return s_revision == UINT32_MAX || atomic_load(&s_diagnostics);
}
bool app_scene_dark(void)
{
    return s_dark;
}
bool app_scene_english(void)
{
    return s_english;
}
