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
    cJSON *second = cJSON_Duplicate(page, true);
    cJSON_ReplaceItemInObject(second, "id", cJSON_CreateString("second"));
    cJSON_AddItemToArray(cJSON_GetObjectItem(s_test_project, "pages"), second);
    s_test_revision++;
    app_scene_poll();
    lv_obj_send_event(lv_obj_get_child(s_root, 151), LV_EVENT_CLICKED, NULL);
    app_scene_poll();
    assert(s_drawn_page == 1);
    lv_obj_send_event(s_toggle, LV_EVENT_CLICKED, NULL);
    assert(lv_obj_has_flag(s_root, LV_OBJ_FLAG_HIDDEN));
    lv_display_set_resolution(display, 800, 1280);
    s_test_revision++;
    app_scene_poll();
    assert(s_points[0][0].x == 80 && s_points[0][0].y == 256);
    puts("LVGL scene: 150 pipes, normalized coordinates, revision reuse, paging, diagnostics OK");
    return 0;
}
