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
cJSON *app_store_project(uint32_t *revision)
{
    *revision = s_test_revision;
    return cJSON_Duplicate(s_test_project, true);
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
    assert(s_runtime[148].rotor && s_runtime[148].rotor_initialized);
    assert(!strcmp(lv_label_get_text(s_runtime[148].power_label), "39 %"));
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
        .bytes = (unsigned char *)"already verified", .owner = 0, .image = {.data_size = 100}};
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
        double px = (.16 + .78 * 15.5 / 16 - .5) * 80;
        double py = (.95 - .5) * 128;
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
        assert(fabs(s_points[0][0].x - (360 - 29.2 * sin(angle))) < 1.01);
        assert(fabs(s_points[0][0].y - (576 + 29.2 * cos(angle))) < 1.01);
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
    lv_point_precise_t frozen[2];
    memcpy(frozen, s_extra_rotors[0].points, sizeof(frozen));
    s_test_power = 10;
    app_scene_update(100);
    assert(!memcmp(frozen, s_extra_rotors[0].points, sizeof(frozen)));
    s_test_valid = false;
    app_scene_update(100);
    assert(s_extra_rotors[0].opacity == LV_OPA_50 && s_extra_rotors[1].opacity == LV_OPA_50);
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
