/* Valódi LVGL objektumok, P4 helyett gazdagépes virtuális kijelző. */
#include <assert.h>
#include "../../main/app_scene.c"
static cJSON *s_test_project;
static uint32_t s_test_revision;
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
    puts("LVGL scene: geometry, paging, activity precedence, power thresholds, reverse flow, "
         "rotor, values, disconnect OK");
    return 0;
}
