/* Valódi LVGL objektumok, P4 helyett gazdagépes virtuális kijelző. */
#include <assert.h>
#include "../../main/app_scene.c"
static cJSON *s_test_project;
static uint32_t s_test_revision;
static unsigned s_brightness_writes;
static int s_test_brightness;
esp_err_t bsp_display_brightness_set(int percent)
{
    s_brightness_writes++;
    s_test_brightness = percent;
    return ESP_OK;
}
bool bsp_display_lock(unsigned timeout)
{
    (void)timeout;
    return true;
}
void bsp_display_unlock(void)
{
}
uint32_t app_store_revision(void)
{
    return s_test_revision;
}
cJSON *app_store_page(int index, int *count, uint32_t *revision)
{
    *revision = s_test_revision;
    return sm_project_page_copy(s_test_project, index, count);
}

static double s_test_activity, s_test_power;
static bool s_test_valid = true;
static bool read_signal(const char *uuid, double *value, void *context)
{
    (void)context;
    *value = uuid[0] == '1' ? s_test_activity : s_test_power;
    return s_test_valid;
}

int main(void)
{
    lv_init();
    lv_display_t *display = lv_display_create(1280, 800);
    assert(display);
    s_test_project = sm_project_defaults();
    s_test_revision = 1;
    app_scene_poll();
    assert(s_root && lv_obj_has_flag(s_root, LV_OBJ_FLAG_HIDDEN));
    lv_obj_send_event(s_toggle, LV_EVENT_CLICKED, NULL);
    assert(!lv_obj_has_flag(s_root, LV_OBJ_FLAG_HIDDEN));
    cJSON *page = cJSON_GetArrayItem(cJSON_GetObjectItem(s_test_project, "pages"), 0);
    cJSON *elements = cJSON_GetObjectItem(page, "elements");
    for (unsigned i = 0; i < 150; i++)
    {
        cJSON *element = cJSON_Parse(
            "{\"type\":\"pipe\",\"x\":0.1,\"y\":0.2,\"w\":0.1,\"h\":0.1,\"color\":\"#ff0000\","
            "\"width\":6,\"points\":[{\"x\":0.1,\"y\":0.2},{\"x\":0.9,\"y\":0.8}]}");
        cJSON_AddItemToArray(elements, element);
    }
    s_test_revision++;
    app_scene_poll();
    assert(lv_obj_get_child_count(s_root) == 152);
    assert(s_points[149][0].x == 128 && s_points[149][0].y == 160);
    assert(s_points[149][1].x == 1152 && s_points[149][1].y == 640);
    lv_obj_t *previous = s_root;
    app_scene_poll();
    assert(previous == s_root);
    cJSON *first = cJSON_GetArrayItem(elements, 0);
    for (unsigned width = 1; width <= 30; width += width == 1 ? 11 : 18)
    {
        cJSON_ReplaceItemInObject(first, "width", cJSON_CreateNumber(width));
        s_test_revision++;
        app_scene_poll();
        assert(lv_obj_get_style_line_width(s_runtime[0].body, 0) == (int)width);
    }
    cJSON_AddItemToObject(
        first,
        "activity",
        cJSON_Parse("{\"uuid\":\"12345678-1234-5678-0102030405060708\",\"scale\":1,\"offset\":0}"));
    cJSON_AddItemToObject(
        first,
        "power",
        cJSON_Parse("{\"source\":{\"uuid\":\"22345678-1234-5678-0102030405060708\",\"scale\":1,"
                    "\"offset\":0},\"minimum\":10,\"maximum\":100}"));
    s_test_revision++;
    app_scene_poll();
    app_scene_set_reader(read_signal, NULL);
    s_test_activity = 1;
    s_test_power = 55;
    app_scene_update(100);
    assert(fabs(s_runtime[0].phase - .05) < .000001);
    assert(!lv_obj_has_flag(s_runtime[0].dots[0], LV_OBJ_FLAG_HIDDEN));
    double phase = s_runtime[0].phase;
    s_test_activity = 0;
    s_test_power = 100;
    app_scene_update(100);
    assert(s_runtime[0].phase == phase);
    assert(lv_obj_has_flag(s_runtime[0].dots[0], LV_OBJ_FLAG_HIDDEN));
    s_test_valid = false;
    app_scene_update(100);
    assert(s_runtime[0].last_opacity == LV_OPA_50);
    s_test_valid = true;
    /* A világos cső sötét, a sötét cső világos áramlásjelzőt kap.
     * A mozgást egy valódi derékszög mindkét oldalán megmérjük. */
    const struct { unsigned width, rgb, marker; } pipes[] = {
        {1, 0xe34d4d, 0xffffff}, {4, 0x287ac4, 0xffffff}, {8, 0x258653, 0xffffff},
        {12, 0xf2cb3d, 0x15202b}, {20, 0xffffff, 0x15202b}, {30, 0x26333d, 0xffffff}
    };
    cJSON *old_points = cJSON_DetachItemFromObject(first, "points");
    cJSON_AddItemToObject(first, "points", cJSON_Parse(
        "[{\"x\":0.1,\"y\":0.2},{\"x\":0.5,\"y\":0.2},{\"x\":0.5,\"y\":0.8}]"));
    s_test_activity = s_test_power = 100;
    for (unsigned i = 0; i < sizeof(pipes)/sizeof(pipes[0]); i++)
    {
        char hex[8];
        snprintf(hex, sizeof(hex), "#%06x", pipes[i].rgb);
        cJSON_ReplaceItemInObject(first, "width", cJSON_CreateNumber(pipes[i].width));
        cJSON_ReplaceItemInObject(first, "color", cJSON_CreateString(hex));
        s_test_revision++;
        app_scene_poll();
        runtime_t *r = &s_runtime[0];
        lv_obj_t *dot = r->dots[0];
        assert(lv_obj_get_style_line_width(r->body, 0) == (int)pipes[i].width);
        assert(lv_color_eq(lv_obj_get_style_line_color(r->body, 0), lv_color_hex(pipes[i].rgb)));
        assert(lv_color_eq(lv_obj_get_style_bg_color(dot, 0), lv_color_hex(pipes[i].marker)));
        assert(!!r->pipe_outline == (i == 4));
        for (int dir = -1; dir <= 1; dir += 2)
        {
            r->direction = dir;
            r->phase = (dir == 1 ? 510 : r->length - 514) / 50;
            app_scene_update(0);
            lv_obj_update_layout(s_root);
            int x = lv_obj_get_x(dot), y = lv_obj_get_y(dot);
            assert(lv_obj_get_width(dot) == 2 * (int)fmax(2,lround(.3*pipes[i].width)));
            app_scene_update(100);
            lv_obj_update_layout(s_root);
            assert(dir * (lv_obj_get_x(dot)-x) > 0 && dir * (lv_obj_get_y(dot)-y) > 0);
        }
    }
    cJSON_ReplaceItemInObject(first, "points", old_points);
    cJSON_DeleteItemFromObject(first, "activity");
    s_test_revision++;
    app_scene_poll();
    s_test_power = 10;
    app_scene_update(100);
    assert(s_runtime[0].phase == 0);
    s_test_power = 11;
    app_scene_update(100);
    assert(fabs(s_runtime[0].phase - .015) < .000001);
    s_runtime[0].direction = -1;
    s_runtime[0].phase = .1;
    app_scene_update(0);
    lv_obj_update_layout(s_root);
    assert(lv_obj_get_x(s_runtime[0].dots[0]) > 600);
    cJSON *second = cJSON_Duplicate(page, true);
    cJSON_ReplaceItemInObject(second, "id", cJSON_CreateString("second"));
    cJSON_AddItemToArray(cJSON_GetObjectItem(s_test_project, "pages"), second);
    s_test_revision++;
    app_scene_poll();
    lv_obj_send_event(
        lv_obj_get_child(s_root, lv_obj_get_child_count(s_root) - 1), LV_EVENT_CLICKED, NULL);
    app_scene_poll();
    assert(s_drawn_page == 1);
    lv_obj_send_event(s_toggle, LV_EVENT_CLICKED, NULL);
    assert(lv_obj_has_flag(s_root, LV_OBJ_FLAG_HIDDEN));
    lv_display_set_resolution(display, 800, 1280);
    s_test_revision++;
    app_scene_poll();
    assert(s_points[0][0].x == 80 && s_points[0][0].y == 256);
    lv_obj_send_event(s_toggle, LV_EVENT_CLICKED, NULL);
    assert(!atomic_load(&s_diagnostics));
    const char *value_json =
        "{\"type\":\"value\",\"x\":0.1,\"y\":0.1,\"w\":0.2,\"h\":0.2,\"color\":\"#ff0000\","
        "\"unit\":\"°C\",\"decimals\":1,\"value\":{\"uuid\":\"22345678-1234-5678-"
        "0102030405060708\",\"scale\":0.5,\"offset\":2}}";
    cJSON_ReplaceItemInArray(cJSON_GetObjectItem(second, "elements"), 149, cJSON_Parse(value_json));
    cJSON_ReplaceItemInArray(
        cJSON_GetObjectItem(second, "elements"),
        148,
        cJSON_Parse("{\"type\":\"equipment\",\"asset\":\"circulation-pump\",\"x\":0.4,\"y\":0.4,"
                    "\"w\":0.1,\"h\":0.1,\"color\":\"#ff0000\",\"showPower\":true,\"power\":{"
                    "\"source\":{\"uuid\":\"22345678-1234-5678-0102030405060708\",\"scale\":1,"
                    "\"offset\":0},\"minimum\":10,\"maximum\":100}}"));
    s_test_revision++;
    app_scene_poll();
    s_test_power = 45;
    app_scene_update(100);
    assert(!strcmp(lv_label_get_text(s_runtime[149].value_label), "24.5 °C"));
    cJSON *aligned_value = cJSON_GetArrayItem(cJSON_GetObjectItem(second, "elements"), 149);
    const char *alignments[] = {NULL, "left", "center", "right"};
    const lv_text_align_t expected_alignments[] = {
        LV_TEXT_ALIGN_LEFT, LV_TEXT_ALIGN_LEFT, LV_TEXT_ALIGN_CENTER, LV_TEXT_ALIGN_RIGHT};
    const unsigned font_sizes[] = {10, 20, 64};
    for (unsigned a = 0; a < 4; a++)
        for (unsigned f = 0; f < 3; f++)
            for (unsigned rotation = 0; rotation < 360; rotation += 90)
            {
                cJSON_DeleteItemFromObject(aligned_value, "align");
                if (alignments[a])
                    cJSON_AddStringToObject(aligned_value, "align", alignments[a]);
                cJSON_DeleteItemFromObject(aligned_value, "fontSize");
                cJSON_AddNumberToObject(aligned_value, "fontSize", font_sizes[f]);
                cJSON_DeleteItemFromObject(aligned_value, "rotation");
                cJSON_AddNumberToObject(aligned_value, "rotation", rotation);
                s_test_revision++;
                app_scene_poll();
                app_scene_update(0);
                lv_obj_t *label = s_runtime[149].value_label;
                assert(lv_obj_get_style_text_align(label, 0) == expected_alignments[a]);
                assert(lv_obj_get_style_transform_rotation(lv_obj_get_parent(label), 0) ==
                       (int)rotation * 10);
                assert(!strcmp(lv_label_get_text(label), "24.5 °C"));
            }
    assert(s_runtime[148].rotor && s_runtime[148].rotor_initialized);
    lv_obj_update_layout(s_root);
    assert(lv_obj_get_width(s_runtime[148].rotor) > 20);
    assert(lv_obj_get_height(s_runtime[148].rotor) > 20);
    for (unsigned n = 0; n < 2; n++)
    {
        assert(s_runtime[148].rotor_points[n].x >= 0 &&
               s_runtime[148].rotor_points[n].x < lv_obj_get_width(s_runtime[148].rotor));
        assert(s_runtime[148].rotor_points[n].y >= 0 &&
               s_runtime[148].rotor_points[n].y < lv_obj_get_height(s_runtime[148].rotor));
    }

    assert(!strcmp(lv_label_get_text(s_runtime[148].power_label), "39 %"));
    cJSON *value_binding = cJSON_GetObjectItem(aligned_value, "value");
    cJSON_ReplaceItemInObject(value_binding, "scale", cJSON_CreateNumber(1));
    cJSON_ReplaceItemInObject(value_binding, "offset", cJSON_CreateNumber(0));
    cJSON_ReplaceItemInObject(aligned_value, "decimals", cJSON_CreateNumber(2));
    s_test_revision++;
    app_scene_poll();
    s_test_power = -.0001;
    app_scene_update(0);
    assert(!strcmp(lv_label_get_text(s_runtime[149].value_label), "0.00 °C"));
    s_test_power = 1e12;
    app_scene_update(0);
    assert(!strcmp(lv_label_get_text(s_runtime[149].value_label), "1e+12 °C"));
    assert(!strcmp(lv_label_get_text(s_runtime[148].power_label), "100 %"));
    cJSON_ReplaceItemInArray(cJSON_GetObjectItem(second, "elements"), 149, cJSON_Parse(value_json));
    s_test_revision++;
    app_scene_poll();
    s_test_valid = false;
    app_scene_update(100);
    assert(!strcmp(lv_label_get_text(s_runtime[148].power_label), "— %"));
    assert(!strcmp(lv_label_get_text(s_runtime[149].value_label), "— °C"));
    cJSON *settings = sm_settings_defaults();
    cJSON *settings_display = cJSON_GetObjectItem(settings, "display");
    cJSON_ReplaceItemInObject(
        settings_display,
        "nightSource",
        cJSON_Parse("{\"uuid\":\"32345678-1234-5678-0102030405060708\",\"scale\":1,\"offset\":0}"));
    cJSON_ReplaceItemInObject(
        cJSON_GetObjectItem(settings, "project"), "language", cJSON_CreateString("en"));
    app_scene_configure(settings);
    cJSON_Delete(settings);
    s_test_valid = true;
    s_test_power = 1;
    app_scene_poll();
    assert(s_dark && s_drawn_dark && s_test_brightness == 15);
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(s_root, 0)) ==
           lv_color_to_u32(lv_color_hex(0x17232f)));
    assert(!strcmp(lv_label_get_text(lv_obj_get_child(s_toggle, 0)), "Info / Schema"));
    previous = s_root;
    unsigned writes = s_brightness_writes;
    app_scene_poll();
    assert(s_root == previous && s_brightness_writes == writes);
    s_test_valid = false;
    app_scene_poll();
    assert(s_dark && s_test_brightness == 15 && s_brightness_writes == writes);
    s_test_valid = true;
    s_test_power = 0;
    cJSON_ReplaceItemInObject(second, "background", cJSON_CreateString("warm"));
    s_test_revision++;
    app_scene_poll();
    assert(!s_dark && s_test_brightness == 85);
    assert(lv_color_to_u32(lv_obj_get_style_bg_color(s_root, 0)) ==
           lv_color_to_u32(lv_color_hex(0xfaf8f1)));
    cJSON_ReplaceItemInObject(second, "background", cJSON_CreateString("plain"));
    s_test_revision++;
    app_scene_poll();
    assert(lv_obj_get_style_bg_grad_dir(s_root, 0) == LV_GRAD_DIR_NONE);
    settings = sm_settings_defaults();
    settings_display = cJSON_GetObjectItem(settings, "display");
    cJSON_ReplaceItemInObject(
        settings_display,
        "nightSource",
        cJSON_Parse("{\"uuid\":\"32345678-1234-5678-0102030405060708\",\"scale\":1,\"offset\":0}"));
    cJSON_ReplaceItemInObject(settings_display, "nightBrightness", cJSON_CreateNumber(0));
    app_scene_configure(settings);
    cJSON_Delete(settings);
    s_test_power = 1;
    app_scene_poll();
    assert(s_dark && s_test_brightness == 0);
    const char *hash = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    cJSON *manifest =
        cJSON_Parse("{\"assetAbi\":1,\"files\":[{\"path\":\"dark/"
                    "circulation-pump.png\",\"size\":100,\"sha256\":"
                    "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}]}");
    picture_t prior = {
        .bytes = (unsigned char *)"already verified", .owner = 0, .image = {.header = {.w = 160, .h = 160}, .data_size = 100}};
    snprintf(prior.digest, sizeof(prior.digest), "%s", hash);
    picture_t shared = load_picture(manifest, "circulation-pump", true, &prior, 1);
    assert(shared.bytes == prior.bytes && shared.owner == 0);
    picture_t absent = load_picture(manifest, "circulation-pump", false, &prior, 1);
    assert(!absent.bytes);
    cJSON_Delete(manifest);
    /* Zárt pumpaháznál nem rajzolunk forgó lapátot a tömör felületre. */
    cJSON *equipment = cJSON_GetArrayItem(cJSON_GetObjectItem(second, "elements"), 148);
    cJSON_ReplaceItemInObject(equipment, "asset", cJSON_CreateString("well-pump"));
    s_test_revision++;
    app_scene_poll();
    assert(s_runtime[148].indicator && !s_runtime[148].rotor);
    s_test_valid = true;
    s_test_power = 45;
    app_scene_update(100);
    assert(s_runtime[148].last_indicator_color == 0x36c678);
    int brightness = s_runtime[148].last_indicator_opacity;
    app_scene_update(100);
    assert(s_runtime[148].last_indicator_opacity != brightness);
    s_test_power = 10;
    app_scene_update(100);
    assert(s_runtime[148].last_indicator_color == 0x596674);
    s_test_valid = false;
    app_scene_update(100);
    assert(s_runtime[148].last_indicator_color == 0xd9a23a);
    assert(s_runtime[148].last_indicator_opacity == LV_OPA_50);
    cJSON *manifold = cJSON_Parse(
        "{\"circuits\":16,\"variant\":\"ceiling-cool\",\"connections\":true,\"heads\":[]}");
    cJSON *heads = cJSON_GetObjectItem(manifold, "heads");
    for (unsigned i = 0; i < 16; i++)
        cJSON_AddItemToArray(heads, cJSON_Parse("{\"percent\":0,\"source\":null}"));
    cJSON *mh = cJSON_GetArrayItem(heads, 0);
    cJSON_ReplaceItemInObject(
        mh, "source",
        cJSON_Parse(
            "{\"uuid\":\"12345678-1234-5678-0102030405060708\",\"scale\":100,\"offset\":0}"));
    cJSON_ReplaceItemInObject(cJSON_GetArrayItem(heads, 1), "percent", cJSON_CreateNumber(25));
    cJSON_ReplaceItemInObject(cJSON_GetArrayItem(heads, 2), "percent", cJSON_CreateNumber(.001));
    cJSON_ReplaceItemInObject(equipment, "asset", cJSON_CreateString("configurable-manifold"));
    cJSON_AddItemToObject(equipment, "manifold", manifold);
    cJSON_AddStringToObject(equipment, "id", "manifold-device");
    cJSON *linked_pipe = cJSON_GetArrayItem(cJSON_GetObjectItem(second, "elements"), 0);
    cJSON_AddItemToObject(linked_pipe, "source",
                          cJSON_Parse("{\"element\":\"manifold-device\",\"port\":33}"));
    for (unsigned rotation = 0; rotation < 360; rotation += 90)
    {
        cJSON_DeleteItemFromObject(equipment, "rotation");
        cJSON_AddNumberToObject(equipment, "rotation", rotation);
        s_test_revision++;
        app_scene_poll();
        assert(s_head_count == 16);
        // Az LVGL alapkonfigurációja egész pixeles pontokat használ.
        double px = ((56 + 56 * 15.5) / 992 - .5) * 80;
        double py = (222. / 224 - .5) * (80. * 224 / 992);
        double angle = rotation * 3.141592653589793 / 180;
        assert(fabs(s_points[0][0].x - (320 + 40 + px * cos(angle) - py * sin(angle))) < 1.01);
        assert(fabs(s_points[0][0].y - (512 + 64 + px * sin(angle) + py * cos(angle))) < 1.01);
        // A szabad végpont változatlan marad.
        assert(s_points[0][1].x == 720 && s_points[0][1].y == 1024);

        s_test_valid = true;
        s_test_activity = .5;
        app_scene_update(50);
        assert(!strcmp(lv_label_get_text(s_heads[0].label), "50%"));
        assert(!strcmp(lv_label_get_text(s_heads[1].label), "25%"));
        assert(s_heads[2].last_color == 0x36c678 && s_heads[3].last_color == 0x26333d);
        s_test_activity = 0;
        app_scene_update(50);
        assert(s_heads[0].last_color == 0x26333d);
        s_test_valid = false;
        app_scene_update(50);
        assert(s_heads[0].last_color == 0xd9a23a);
        assert(!strcmp(lv_label_get_text(s_heads[0].label), "—"));
        assert(s_heads[1].last_color == 0x36c678);
    }
    cJSON_DeleteItemFromObject(equipment, "manifold");
    cJSON_ReplaceItemInObject(equipment, "asset", cJSON_CreateString("motor-valve-3"));
    cJSON_ReplaceItemInObject(cJSON_GetObjectItem(linked_pipe, "source"), "port",
                              cJSON_CreateNumber(2));
    for (unsigned rotation = 0; rotation < 360; rotation += 90)
    {
        cJSON_ReplaceItemInObject(equipment, "rotation", cJSON_CreateNumber(rotation));
        s_test_revision++;
        app_scene_poll();
        double angle = rotation * 3.141592653589793 / 180;
        assert(s_head_count == 0);
        assert(fabs(s_points[0][0].x - (360 - 31.5 * sin(angle))) < 1.01);
        assert(fabs(s_points[0][0].y - (576 + 31.5 * cos(angle))) < 1.01);
    }
    // Minden katalógusport: a négyzetes kép illesztése fekvő és álló vásznon, négy irányban.
    for (unsigned portrait = 0; portrait < 2; portrait++)
        for (unsigned a = 0; a < sm_catalog_count; a++)
        {
            const sm_asset_t *asset = &sm_catalog[a];
            cJSON_ReplaceItemInObject(equipment, "asset", cJSON_CreateString(asset->id));
            int width = portrait ? 800 : 1280, height = portrait ? 1280 : 800;
            int x = sm_json_number(equipment, "x", 0) * width;
            int y = sm_json_number(equipment, "y", 0) * height;
            int w = sm_json_number(equipment, "w", .1) * width;
            int h = sm_json_number(equipment, "h", .1) * height;
            unsigned n = sm_manifold_raster_circuits(asset->id);
            double ratio = n ? (96 + 56*n)/224. : 1;
            double image_w = fmin(w,h*ratio), image_h = image_w/ratio;
            for (unsigned rotation = 0; rotation < 360; rotation += 90)
            {
                cJSON_ReplaceItemInObject(equipment, "rotation", cJSON_CreateNumber(rotation));
                for (unsigned p = 0; p < asset->port_count; p++)
                {
                    lv_point_precise_t point;
                    assert(equipment_port_position(equipment, p, width, height, &point));
                    double dx = (asset->ports[p].x - .5) * image_w;
                    double dy = (asset->ports[p].y - .5) * image_h;
                    for (unsigned turn = 0; turn < rotation / 90; turn++)
                    {
                        double previous_dx = dx;
                        dx = -dy;
                        dy = previous_dx;
                    }
                    assert(fabs(point.x - (x + w / 2.0 + dx)) < 1.01);
                    assert(fabs(point.y - (y + h / 2.0 + dy)) < 1.01);
                }
                lv_point_precise_t invalid;
                assert(!equipment_port_position(equipment, asset->port_count, width, height, &invalid));
            }
        }
    cJSON_ReplaceItemInObject(equipment, "asset", cJSON_CreateString("dry-cooler"));
    cJSON_ReplaceItemInObject(equipment, "rotation", cJSON_CreateNumber(90));
    cJSON_ReplaceItemInObject(cJSON_GetObjectItem(linked_pipe, "source"), "port",
                              cJSON_CreateNumber(0));
    s_test_revision++;
    app_scene_poll();
    s_test_valid = true;
    s_test_power = 50;
    app_scene_update(100);
    assert(s_extra_rotor_count == 2 && s_extra_rotors[0].initialized &&
           s_extra_rotors[1].initialized);
    assert(s_extra_rotors[0].scale_y == sm_asset_find("dry-cooler")->rotor_scale_y);
    assert(s_extra_rotors[0].scale_y < .3);
    lv_obj_update_layout(s_root);
    assert(lv_obj_get_width(s_extra_rotors[0].object) >= 18);
    assert(lv_obj_get_height(s_extra_rotors[0].object) >= 18);
    assert(lv_color_to_u32(lv_obj_get_style_line_color(s_extra_rotors[0].object, 0)) ==
           lv_color_to_u32(lv_color_hex(0x9cb0be)));

    lv_point_precise_t frozen[2];
    memcpy(frozen, s_extra_rotors[0].points, sizeof(frozen));
    s_test_power = 10;
    app_scene_update(100);
    assert(!memcmp(frozen, s_extra_rotors[0].points, sizeof(frozen)));
    s_test_valid = false;
    app_scene_update(100);
    assert(s_extra_rotors[0].opacity == LV_OPA_50 && s_extra_rotors[1].opacity == LV_OPA_50);
    const char *sprinklers[] = {"rotor-sprinkler", "spray-sprinkler", "micro-sprinkler",
                                "rotary-nozzle", "impact-sprinkler"};
    cJSON_DeleteItemFromObject(equipment, "activity");
    cJSON_AddItemToObject(equipment, "activity", cJSON_Parse(
        "{\"uuid\":\"12345678-1234-5678-0102030405060708\",\"scale\":1,\"offset\":0}"));
    for (unsigned kind = 0; kind < 5; kind++)
        for (unsigned rotation = 0; rotation < 360; rotation += 90)
        {
            cJSON_ReplaceItemInObject(equipment, "asset", cJSON_CreateString(sprinklers[kind]));
            cJSON_ReplaceItemInObject(equipment, "rotation", cJSON_CreateNumber(rotation));
            s_test_revision++;
            app_scene_poll();
            assert(s_spray_count == 1 && s_extra_rotor_count == 0);
            spray_runtime_t *spray = &s_sprays[0];
            assert(spray->owner == 148 && !spray->visible);
            s_test_valid = true;
            s_test_activity = 1;
            s_test_power = 100;
            app_scene_update(100);
            lv_obj_update_layout(s_root);
            assert(spray->visible && s_runtime[148].last_indicator_color == 0x36c678);
            for (unsigned drop = 0; drop < SM_SPRAY_DROPS; drop++)
            {
                assert(!lv_obj_has_flag(spray->drops[drop], LV_OBJ_FLAG_HIDDEN));
                double dx, dy;
                assert(sm_spray_point(s_runtime[148].phase, drop, spray->direction, &dx, &dy));
                double x = spray->x + spray->radius * (dx * cos(spray->rotation) - dy * sin(spray->rotation));
                double y = spray->y + spray->radius * (dx * sin(spray->rotation) + dy * cos(spray->rotation));
                assert(fabs(lv_obj_get_x(spray->drops[drop]) + spray->half - x) < 1.01);
                assert(fabs(lv_obj_get_y(spray->drops[drop]) + spray->half - y) < 1.01);
            }
            double frozen_phase = s_runtime[148].phase;
            atomic_store(&s_diagnostics, true);
            app_scene_update(100);
            assert(s_runtime[148].phase == frozen_phase);
            atomic_store(&s_diagnostics, false);
            s_test_activity = 0; // Az explicit kikapcsolás a pozitív teljesítményt is felülírja.
            app_scene_update(100);
            assert(!spray->visible && s_runtime[148].phase == frozen_phase);
            for (unsigned drop = 0; drop < SM_SPRAY_DROPS; drop++)
                assert(lv_obj_has_flag(spray->drops[drop], LV_OBJ_FLAG_HIDDEN));
            assert(s_runtime[148].last_indicator_color == 0x596674);
            s_test_valid = false;
            app_scene_update(100);
            assert(!spray->visible && s_runtime[148].last_indicator_color == 0xd9a23a);
        }
    cJSON_DeleteItemFromObject(equipment, "activity");
    s_test_revision++;
    app_scene_poll();
    s_test_valid = true;
    s_test_power = 55;
    app_scene_update(100);
    assert(s_sprays[0].visible && fabs(s_runtime[148].phase - .05) < 1e-12);
    s_test_power = 10;
    app_scene_update(100);
    assert(!s_sprays[0].visible);
    cJSON *spray_elements = cJSON_GetObjectItem(second, "elements");
    for (unsigned index = 0; index < SM_MAX_ELEMENTS; index++)
        cJSON_ReplaceItemInArray(spray_elements, index, cJSON_Parse(
            "{\"type\":\"equipment\",\"asset\":\"spray-sprinkler\",\"x\":0.2,\"y\":0.2,"
            "\"w\":0.1,\"h\":0.1,\"color\":\"#ff0000\",\"activity\":{"
            "\"uuid\":\"12345678-1234-5678-0102030405060708\",\"scale\":1,\"offset\":0}}"));
    s_test_revision++;
    app_scene_poll();
    s_test_activity = 1;
    app_scene_update(100);
    assert(s_spray_count == SM_MAX_ELEMENTS);
    lv_obj_t *last_drop = s_sprays[SM_MAX_ELEMENTS - 1].drops[SM_SPRAY_DROPS - 1];
    for (unsigned tick = 0; tick < 20; tick++)
        app_scene_update(50);
    assert(last_drop == s_sprays[SM_MAX_ELEMENTS - 1].drops[SM_SPRAY_DROPS - 1]);
    assert(!lv_obj_has_flag(last_drop, LV_OBJ_FLAG_HIDDEN));
    s_test_valid = false;
    app_scene_update(50);
    for (unsigned index = 0; index < SM_MAX_ELEMENTS; index++)
        for (unsigned drop = 0; drop < SM_SPRAY_DROPS; drop++)
            assert(lv_obj_has_flag(s_sprays[index].drops[drop], LV_OBJ_FLAG_HIDDEN));
    puts("LVGL scene: geometry, paging, activity precedence, power thresholds, reverse flow, "
         "rotor, values, disconnect, theme, brightness and shared PNG cache OK");
    return 0;
}

/* A fájltár integrációját a külön natív SD-próba ellenőrzi. */
cJSON *app_assets_manifest(void)
{
    return NULL;
}
bool app_assets_path(const cJSON *manifest, const char *relative, char path[APP_ASSET_PATH_SIZE])
{
    (void)manifest;
    (void)relative;
    (void)path;
    return false;
}

uint32_t app_assets_revision(void)
{
    return 0;
}
