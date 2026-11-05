/* A valódi LVGL diagnosztikai nézete, virtuális kijelzőn. */
#include <assert.h>
#include "../../main/app_diagnostic_view.c"
static bool visible = true, dark, english;
static unsigned locks;
bool app_scene_diagnostics_visible(void)
{
    return visible;
}
bool app_scene_dark(void)
{
    return dark;
}
bool app_scene_english(void)
{
    return english;
}
bool bsp_display_lock(unsigned timeout)
{
    (void)timeout;
    locks++;
    return true;
}
void bsp_display_unlock(void)
{
}
void app_diagnostics_capture(app_diagnostics_t *out)
{
    memset(out, 0, sizeof(*out));
}
int64_t esp_timer_get_time(void)
{
    return 0;
}
int main(void)
{
    lv_init();
    lv_display_t *display = lv_display_create(1280, 800);
    assert(display);
    app_diagnostic_view_poll();
    assert(s_root && !lv_obj_has_flag(s_root, LV_OBJ_FLAG_HIDDEN));
    assert(strstr(lv_label_get_text(s_labels[0]), "Mérés"));
    for (unsigned i = 0; i < 5; i++)
        assert(lv_obj_has_flag(s_bars[i], LV_OBJ_FLAG_HIDDEN));
    app_diagnostics_t data = {.sampled = true,
                              .sd_mounted = true,
                              .internal_free = 64000,
                              .internal_total = 256000,
                              .internal_minimum = 16000,
                              .psram_total = 33554432,
                              .psram_free = 30000000,
                              .main_stack_free = 4096,
                              .task_count = 23,
                              .numeric_age = -1,
                              .recovery_wifi = true};
    snprintf(data.firmware, sizeof(data.firmware), "native-test");
    snprintf(data.recovery_ssid, sizeof(data.recovery_ssid), "ZoleeSchema-Test");
    app_network_status_t network = {.recovery = true};
    snprintf(network.recovery_ssid, sizeof(network.recovery_ssid), "ZoleeSchema-Test");
    snprintf(network.recovery_password, sizeof(network.recovery_password), "NativeOnlySecret");
    app_diagnostic_view_update(&data, &network);
    app_diagnostic_view_poll();
    assert(strstr(lv_label_get_text(s_labels[11]), "NativeOnlySecret"));
    assert(strstr(lv_label_get_text(s_labels[3]), "nem mért"));
    assert(lv_obj_has_flag(s_bars[0], LV_OBJ_FLAG_HIDDEN));
    unsigned unchanged = locks;
    app_diagnostic_view_poll();
    assert(unchanged == locks);
    data.sampled = false;
    app_diagnostic_view_update(&data, &network);
    app_diagnostic_view_poll();
    assert(strstr(lv_label_get_text(s_labels[11]), "NativeOnlySecret"));
    data.sampled = true;
    data.sample_age = 7;
    app_diagnostic_view_update(&data, &network);
    app_diagnostic_view_poll();
    assert(strstr(lv_label_get_text(s_note), "7 s"));
    data.sample_age = 0;
    data.cpu_valid = true;
    data.cpu[0] = 90;
    data.cpu[1] = 25;
    data.sd_mounted = data.sd_space_valid = true;
    data.sd_total = 34359738368ULL;
    data.sd_free = data.sd_total / 2;
    data.event_count = 1;
    data.events[0] = (app_event_t){.code = APP_EVENT_STACK_LOW, .uptime_seconds = 3661};
    data.numeric_age = 0;
    data.known_values = 42;
    snprintf(data.update_phase, sizeof(data.update_phase), "graphics");
    app_diagnostic_view_update(&data, &network);
    app_diagnostic_view_poll();
    assert(lv_bar_get_value(s_bars[0]) == 90 && lv_bar_get_value(s_bars[1]) == 25);
    assert(lv_bar_get_value(s_bars[4]) == 50 && !lv_obj_has_flag(s_bars[4], LV_OBJ_FLAG_HIDDEN));
    assert(strstr(lv_label_get_text(s_logs[0]), "01:01:01") && s_levels[0] == 1);
    assert(strstr(lv_label_get_text(s_labels[6]), "0 s"));
    visible = false;
    app_diagnostic_view_poll();
    assert(lv_obj_has_flag(s_root, LV_OBJ_FLAG_HIDDEN));
    data.cpu[0] = 50;
    app_diagnostic_view_update(&data, &network);
    unchanged = locks;
    app_diagnostic_view_poll();
    assert(unchanged == locks);
    visible = true;
    app_diagnostic_view_poll();
    assert(lv_bar_get_value(s_bars[0]) == 50);
    dark = english = true;
    app_diagnostic_view_poll();
    assert(strstr(lv_label_get_text(s_labels[11]), "Password"));
    assert(lv_color_eq(lv_obj_get_style_bg_color(s_root, 0), lv_color_hex(0x111d27)));
    lv_display_set_resolution(display, 800, 1280);
    app_diagnostic_view_poll();
    lv_obj_update_layout(s_root);
    assert(lv_obj_get_width(lv_obj_get_child(s_root, 2)) == 374);
    assert(lv_obj_get_height(lv_obj_get_child(s_root, 15)) > 390);
    data.cpu_valid = data.sd_space_valid = data.recovery_wifi = false;
    network.recovery = false;
    app_diagnostic_view_update(&data, &network);
    app_diagnostic_view_poll();
    assert(lv_obj_has_flag(s_bars[0], LV_OBJ_FLAG_HIDDEN) &&
           lv_obj_has_flag(s_bars[4], LV_OBJ_FLAG_HIDDEN));
    assert(!strstr(lv_label_get_text(s_labels[11]), "NativeOnlySecret"));
    puts("Native diagnostics view: unknown values, bars, unchanged/hidden updates, local-only "
         "password, themes, HU/EN and portrait layout OK");
}
