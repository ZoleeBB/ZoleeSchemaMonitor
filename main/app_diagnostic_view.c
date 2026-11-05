/* Memóriasávok, állapotkártyák és 15 soros napló a testvérprojekt nyomán.
 * IO nincs a rajzolóban. Téma/nyelv/méretváltáskor épül újra a nézet. */
#include "app_diagnostic_view.h"
#include "app_scene.h"
#include "bsp/esp-bsp.h"
#include "lvgl.h"
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(app_font_20);
#ifdef ESP_PLATFORM
#include "esp_attr.h"
#define DIAG_RAM EXT_RAM_BSS_ATTR
#else
#define DIAG_RAM
#endif
static lv_obj_t *s_note;
static char s_note_text[128];
static lv_obj_t *s_root, *s_labels[12], *s_bars[5], *s_logs[APP_EVENT_LIMIT];
static DIAG_RAM char s_values[12][256], s_lines[APP_EVENT_LIMIT][192], s_password[17];
static unsigned s_levels[APP_EVENT_LIMIT];
static DIAG_RAM app_diagnostics_t s_data;
static bool s_dark, s_english, s_visible, s_dirty = true;
static int s_width, s_height;
static const char *s_titles[12][2] = {{"CPU-terhelés", "CPU load"},
                                      {"Belső RAM", "Internal RAM"},
                                      {"PSRAM", "PSRAM"},
                                      {"SD-kártya", "SD card"},
                                      {"Wi-Fi / web", "Wi-Fi / web"},
                                      {"Loxone", "Loxone"},
                                      {"Számszerű Loxone-adat", "Numeric Loxone data"},
                                      {"DMA-memória", "DMA memory"},
                                      {"Rendszer", "System"},
                                      {"Feladatok / verem", "Tasks / stack"},
                                      {"Kiadásfrissítés", "Release update"},
                                      {"Beállító Wi-Fi", "Setup Wi-Fi"}};
static const char *tr(const char *hu, const char *en)
{
    return s_english ? en : hu;
}
static void hidden(lv_obj_t *object, bool hide)
{
    if (lv_obj_has_flag(object, LV_OBJ_FLAG_HIDDEN) == hide)
        return;
    if (hide)
        lv_obj_add_flag(object, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_remove_flag(object, LV_OBJ_FLAG_HIDDEN);
}
static lv_obj_t *label(lv_obj_t *parent, const char *text, int x, int y, int width)
{
    lv_obj_t *object = lv_label_create(parent);
    lv_label_set_text_static(object, text);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_width(object, width);
    lv_label_set_long_mode(object, LV_LABEL_LONG_MODE_DOTS);
    return object;
}
static void value(unsigned index, const char *text)
{
    if (!strcmp(s_values[index], text))
        return;
    snprintf(s_values[index], sizeof(s_values[index]), "%s", text);
    lv_label_set_text_static(s_labels[index], s_values[index]);
}
static void bar(unsigned index, bool valid, unsigned percent)
{
    hidden(s_bars[index], !valid);
    if (!valid)
        return;
    if (percent > 100)
        percent = 100;
    if (lv_bar_get_value(s_bars[index]) != (int)percent)
        lv_bar_set_value(s_bars[index], percent, LV_ANIM_OFF);
    lv_color_t color = lv_color_hex(percent >= 90 ? 0xe26058 : percent >= 75 ? 0xe0a43b : 0x2998c4);
    if (!lv_color_eq(lv_obj_get_style_bg_color(s_bars[index], LV_PART_INDICATOR), color))
        lv_obj_set_style_bg_color(s_bars[index], color, LV_PART_INDICATOR);
}
static unsigned used(uint64_t total, uint64_t free_bytes)
{
    return total && free_bytes <= total ? (unsigned)((total - free_bytes) * 100 / total) : 0;
}
static void build(int width, int height)
{
    if (s_root)
        lv_obj_delete(s_root);
    s_root = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(s_root);
    lv_obj_set_size(s_root, width, height);
    lv_obj_remove_flag(s_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(s_root, lv_color_hex(s_dark ? 0x111d27 : 0xf0f4f7), 0);
    lv_obj_set_style_bg_opa(s_root, LV_OPA_COVER, 0);
    lv_obj_set_style_text_font(s_root, &app_font_20, 0);
    lv_obj_set_style_text_color(s_root, lv_color_hex(s_dark ? 0xe5eef5 : 0x233d50), 0);
    label(s_root, tr("Rendszerdiagnosztika", "System diagnostics"), 20, 16, width - 210);
    s_note = label(s_root, s_note_text, 20, 48, width - 40);
    int columns = width >= 1000 ? 3 : 2, gap = 12;
    int cell_width = (width - 40 - gap * (columns - 1)) / columns;
    int rows = (12 + columns - 1) / columns;
    int line_height = app_font_20.line_height;
    int value_y = line_height + 4;
    int card_height = value_y + 3 * line_height + 6;
    int row_height = card_height + gap;
    for (unsigned i = 0; i < 12; i++)
    {
        lv_obj_t *card = lv_obj_create(s_root);
        lv_obj_remove_style_all(card);
        lv_obj_set_pos(card, 20 + (i % columns) * (cell_width + gap), 86 + (i / columns) * row_height);
        lv_obj_set_size(card, cell_width, card_height);
        lv_obj_remove_flag(card, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(card, lv_color_hex(s_dark ? 0x203342 : 0xffffff), 0);
        lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
        lv_obj_set_style_radius(card, 10, 0);
        lv_obj_t *title = label(card, s_titles[i][s_english], 12, 4, cell_width - 24);
        lv_obj_set_style_text_color(title, lv_color_hex(s_dark ? 0xadc5d5 : 0x5f7889), 0);
        s_labels[i] = label(card, s_values[i], 12, value_y, cell_width - 24);
        lv_obj_set_height(s_labels[i], (i >= 4 ? 3 : 2) * line_height);
        if (i < 4)
        {
            unsigned start = i ? i + 1 : 0, count = i ? 1 : 2;
            for (unsigned n = 0; n < count; n++)
            {
                s_bars[start + n] = lv_bar_create(card);
                lv_obj_set_pos(s_bars[start + n], 12, n ? card_height - 12 : card_height - 26);
                lv_obj_set_size(s_bars[start + n], cell_width - 24, 8);
                lv_bar_set_range(s_bars[start + n], 0, 100);
            }
        }
    }
    int log_y = 86 + rows * row_height;
    label(s_root,
          tr("Legutóbbi események · üzemidő szerint", "Recent events · uptime timestamps"),
          20,
          log_y,
          width - 40);
    lv_obj_t *log = lv_obj_create(s_root);
    lv_obj_remove_style_all(log);
    lv_obj_set_pos(log, 20, log_y + 30);
    lv_obj_set_size(log, width - 40, height - log_y - 42);
    lv_obj_set_scroll_dir(log, LV_DIR_VER);
    for (unsigned i = 0; i < APP_EVENT_LIMIT; i++)
    {
        s_logs[i] = label(log, s_lines[i], 0, i * (line_height + 2), width - 60);
        s_levels[i] = 99;
    }
}
static void render(void)
{
    const app_diagnostics_t *d = &s_data;
    char text[256];
    if (d->sampled && d->sample_age > 3)
        snprintf(text,
                 sizeof(text),
                 "%s: %llu s",
                 tr("Utolsó minta kora", "Last sample age"),
                 (unsigned long long)d->sample_age);
    else
        snprintf(text,
                 sizeof(text),
                 "%s",
                 tr("Helyi állapot és eseménynapló", "Local status and event log"));
    if (strcmp(text, s_note_text))
    {
        snprintf(s_note_text, sizeof(s_note_text), "%.127s", text);
        lv_label_set_text_static(s_note, s_note_text);
    }
    if (d->cpu_valid)
        snprintf(text, sizeof(text), "CPU0 %u%% / CPU1 %u%%", d->cpu[0], d->cpu[1]);
    else
        snprintf(text, sizeof(text), "%s", tr("Mérés indul...", "Starting measurement..."));
    value(0, text);
    bar(0, d->cpu_valid, d->cpu[0]);
    bar(1, d->cpu_valid, d->cpu[1]);
    for (unsigned i = 1; i <= 2; i++)
    {
        unsigned available = i == 1 ? d->internal_free : d->psram_free;
        unsigned total = i == 1 ? d->internal_total : d->psram_total;
        if (!d->sampled || !total)
            snprintf(text, sizeof(text), "%s", tr("Nem mért", "Not measured"));
        else if (i == 1)
            snprintf(text,
                     sizeof(text),
                     "%.1f / %.1f KiB %s\n%s: %.1f KiB",
                     available / 1024.0,
                     total / 1024.0,
                     tr("szabad", "free"),
                     tr("Mélypont", "Minimum"),
                     d->internal_minimum / 1024.0);
        else
            snprintf(text,
                     sizeof(text),
                     "%.1f / %.1f MiB %s",
                     available / 1048576.0,
                     total / 1048576.0,
                     tr("szabad", "free"));
        value(i, text);
        bar(i + 1, d->sampled && total > 0, used(total, available));
    }
    if (d->sd_space_valid)
        snprintf(text,
                 sizeof(text),
                 "%.2f / %.2f GiB %s%s",
                 d->sd_free / 1073741824.0,
                 d->sd_total / 1073741824.0,
                 tr("szabad", "free"),
                 d->storage_recovery ? tr("\nMentés visszaállítva", "\nState recovered") : "");
    else
        snprintf(text,
                 sizeof(text),
                 "%s",
                 d->sampled && !d->sd_mounted ? tr("Nem érhető el", "Unavailable")
                                              : tr("Kapacitás nem mért", "Capacity not measured"));
    value(3, text);
    bar(4, d->sd_space_valid, used(d->sd_total, d->sd_free));
    if (!d->sampled)
        value(4, tr("Nem mért", "Not measured"));
    else
    {
        snprintf(text,
                 sizeof(text),
                 "%.32s\n%s%.15s",
                 d->wifi_connected ? d->ssid : tr("Nincs kapcsolat", "Disconnected"),
                 d->ip[0] ? "http://" : "",
                 d->ip);
        value(4, text);
    }
    if (!d->sampled)
        value(5, tr("Nem mért", "Not measured"));
    else
    {
        snprintf(text,
                 sizeof(text),
                 "%s\nWi-Fi: %s",
                 d->loxone_connected ? tr("Kapcsolódva", "Connected")
                                     : tr("Nincs kapcsolat", "Disconnected"),
                 d->wifi_connected ? "—" : tr("nincs kapcsolat", "disconnected"));
        if (d->wifi_connected && d->rssi_valid)
            snprintf(text,
                     sizeof(text),
                     "%s\nWi-Fi: %d dBm",
                     d->loxone_connected ? tr("Kapcsolódva", "Connected")
                                         : tr("Nincs kapcsolat", "Disconnected"),
                     d->rssi);
        value(5, text);
    }
    if (!d->sampled || d->numeric_age < 0)
        value(6, tr("Még nincs érvényes adat", "No valid data yet"));
    else
    {
        snprintf(text,
                 sizeof(text),
                 "%s: %lld s\n%s: %u",
                 tr("Utolsó adat", "Last update"),
                 (long long)d->numeric_age,
                 tr("Ismert értékek", "Known values"),
                 d->known_values);
        value(6, text);
    }
    if (!d->sampled)
        value(7, tr("Nem mért", "Not measured"));
    else
    {
        snprintf(text,
                 sizeof(text),
                 "%s: %.1f KiB\n%s: %.1f KiB",
                 tr("Szabad", "Free"),
                 d->dma_free / 1024.0,
                 tr("Legnagyobb blokk", "Largest block"),
                 d->dma_largest / 1024.0);
        value(7, text);
    }
    if (!d->sampled)
        value(8, tr("Mérés indul...", "Starting measurement..."));
    else
    {
        snprintf(text,
                 sizeof(text),
                 "%.32s\n%lluh %02llum %02llus\n%s",
                 d->firmware,
                 (unsigned long long)(d->uptime_seconds / 3600),
                 (unsigned long long)(d->uptime_seconds / 60 % 60),
                 (unsigned long long)(d->uptime_seconds % 60),
                 d->clock_valid ? tr("Óra beállítva", "Clock valid")
                                : tr("Órára vár", "Waiting for clock"));
        value(8, text);
    }
    if (!d->sampled)
        value(9, tr("Nem mért", "Not measured"));
    else
    {
        snprintf(text,
                 sizeof(text),
                 "%s: %u\napp_main: %u B %s",
                 tr("Feladatok", "Tasks"),
                 d->task_count,
                 d->main_stack_free,
                 tr("maradék", "minimum free"));
        value(9, text);
    }
    if (!d->sampled)
        value(10, tr("Nem mért", "Not measured"));
    else
    {
        snprintf(text,
                 sizeof(text),
                 "%s\n%.64s\n%.2f MiB",
                 app_diagnostics_phase_text(d->update_phase, s_english),
                 d->update_version,
                 d->update_received / 1048576.0);
        value(10, text);
    }
    /* A beállító hálózat kulcsa a sampler hibája mellett is elérhető marad. */
    if (d->recovery_wifi)
        snprintf(text,
                 sizeof(text),
                 "%.32s\n%s: %.16s",
                 d->recovery_ssid,
                 tr("Jelszó", "Password"),
                 s_password);
    else if (!d->sampled)
        snprintf(text, sizeof(text), "%s", tr("Nem mért", "Not measured"));
    else
        snprintf(text, sizeof(text), "%s", tr("Inaktív", "Inactive"));
    value(11, text);
    for (unsigned i = 0; i < APP_EVENT_LIMIT; i++)
    {
        if (i < d->event_count)
        {
            const app_event_t *event = &d->events[i];
            snprintf(text,
                     sizeof(text),
                     "%02llu:%02llu:%02llu  %s",
                     (unsigned long long)(event->uptime_seconds / 3600),
                     (unsigned long long)(event->uptime_seconds / 60 % 60),
                     (unsigned long long)(event->uptime_seconds % 60),
                     app_event_text(event->code, s_english));
        }
        else
            snprintf(
                text, sizeof(text), "%s", i == 0 ? tr("Még nincs esemény.", "No events yet.") : "");
        if (strcmp(s_lines[i], text))
        {
            snprintf(s_lines[i], sizeof(s_lines[i]), "%.*s", (int)sizeof(s_lines[i]) - 1, text);
            lv_label_set_text_static(s_logs[i], s_lines[i]);
        }
        unsigned level = i < d->event_count ? app_event_level(d->events[i].code) : 0;
        if (level != s_levels[i])
        {
            lv_obj_set_style_text_color(s_logs[i],
                                        lv_color_hex(level == 2   ? 0xe26058
                                                     : level == 1 ? 0xd69d32
                                                     : s_dark     ? 0xe5eef5
                                                                  : 0x233d50),
                                        0);
            s_levels[i] = level;
        }
        hidden(s_logs[i], i >= d->event_count && i != 0);
    }
}
void app_diagnostic_view_update(const app_diagnostics_t *data, const app_network_status_t *network)
{
    s_data = *data;
    s_data.recovery_wifi = network->recovery;
    snprintf(s_data.recovery_ssid, sizeof(s_data.recovery_ssid), "%s", network->recovery_ssid);
    snprintf(
        s_password, sizeof(s_password), "%s", network->recovery ? network->recovery_password : "");
    s_dirty = true;
}
void app_diagnostic_view_poll(void)
{
    bool visible = app_scene_diagnostics_visible(), dark = app_scene_dark(),
         english = app_scene_english();
    int width = lv_display_get_horizontal_resolution(NULL),
        height = lv_display_get_vertical_resolution(NULL);
    bool rebuild =
        !s_root || dark != s_dark || english != s_english || width != s_width || height != s_height;
    if (!rebuild && visible == s_visible && (!s_dirty || !visible))
        return;
    if (!bsp_display_lock(100))
        return;
    s_dark = dark;
    s_english = english;
    s_width = width;
    s_height = height;
    if (rebuild)
        build(width, height);
    hidden(s_root, !visible);
    s_visible = visible;
    if (visible)
    {
        render();
        s_dirty = false;
    }
    bsp_display_unlock();
}
