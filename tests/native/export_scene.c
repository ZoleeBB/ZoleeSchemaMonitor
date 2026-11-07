/* Linux/gazdagépes próba az exportált sémákhoz. A CPU és az SD helyi. */
#define _XOPEN_SOURCE 700
#include <assert.h>
#include <malloc.h>
#include <time.h>
#include <ucontext.h>
#include "../../main/app_scene.c"

#if !LV_USE_LODEPNG || LV_CACHE_DEF_SIZE != 4194304
#error "A próbához PNG-dekóderes, 4 MiB cache-es LVGL build szükséges"
#endif

enum
{
    HOST_STACK = 128 * 1024,
    FRAMES = 20
};
static const char *graphics;
static cJSON *project;
static lv_display_t *display;
static unsigned revision, frame, flushes;
static size_t allocated, peak;
static bool connected;
static double raw;
static uint16_t *pixels;
static int width, height;

/* A linkelt alkalmazás és az LVGL lefoglalt blokkjait követjük (glibc). */
void *__real_malloc(size_t);
void *__real_calloc(size_t, size_t);
void *__real_realloc(void *, size_t);
void *__real_aligned_alloc(size_t, size_t);
void __real_free(void *);
static void *track(void *p)
{
    if (p)
    {
        allocated += malloc_usable_size(p);
        if (allocated > peak)
            peak = allocated;
    }
    return p;
}
void *__wrap_malloc(size_t size)
{
    return track(__real_malloc(size));
}
void *__wrap_calloc(size_t n, size_t size)
{
    return track(__real_calloc(n, size));
}
void *__wrap_aligned_alloc(size_t align, size_t size)
{
    return track(__real_aligned_alloc(align, size));
}
void *__wrap_realloc(void *p, size_t size)
{
    size_t previous = p ? malloc_usable_size(p) : 0;
    void *next = __real_realloc(p, size);
    if (next || !size)
        allocated -= previous;
    return track(next);
}
void __wrap_free(void *p)
{
    if (p)
        allocated -= malloc_usable_size(p);
    __real_free(p);
}

static cJSON *read_json(const char *path)
{
    FILE *file = fopen(path, "rb");
    assert(file && fseek(file, 0, SEEK_END) == 0);
    long length = ftell(file);
    assert(length > 0 && length <= 1048576);
    rewind(file);
    char *data = malloc((size_t)length + 1);
    assert(data && fread(data, 1, length, file) == (size_t)length);
    data[length] = 0;
    fclose(file);
    cJSON *json = cJSON_Parse(data);
    free(data);
    assert(json);
    return json;
}
esp_err_t bsp_display_brightness_set(int percent)
{
    (void)percent;
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
    return revision;
}
cJSON *app_store_page(int index, int *count, uint32_t *value)
{
    *value = revision;
    return sm_project_page_copy(project, index, count);
}
uint32_t app_assets_revision(void)
{
    return 1;
}
bool app_assets_path(const cJSON *manifest, const char *relative, char path[APP_ASSET_PATH_SIZE])
{
    (void)manifest;
    int n = snprintf(path, APP_ASSET_PATH_SIZE, "%s/%s", graphics, relative);
    return n >= 0 && n < APP_ASSET_PATH_SIZE;
}
cJSON *app_assets_manifest(void)
{
    char path[APP_ASSET_PATH_SIZE];
    assert(app_assets_path(NULL, "manifest.json", path));
    return read_json(path);
}

static bool demo_read(const char *uuid, double *value, void *context)
{
    (void)context;
    assert(!strcmp(uuid, "12345678-1234-5678-0102030405060708"));
    *value = raw;
    return connected;
}
static void flush(lv_display_t *d, const lv_area_t *area, uint8_t *bytes)
{
    int span = lv_area_get_width(area);
    for (int y = area->y1; y <= area->y2; y++)
    {
        memcpy(pixels + y * width + area->x1, bytes, span * sizeof(*pixels));
        bytes += span * sizeof(*pixels);
    }
    flushes++;
    lv_display_flush_ready(d);
}
static double milliseconds(void);
static double frame_ms;
static void update(void)
{
    double start = milliseconds();
    app_scene_poll();
    app_scene_update(50);
    frame_ms = milliseconds() - start;
}
static void render(void)
{
    double start = milliseconds();
    lv_tick_inc(50);
    lv_refr_now(display);
    frame_ms += milliseconds() - start;
}
/* A festett verem mélypontja a külső kód (libc, PNG, LVGL) hívásait is tartalmazza. */
static size_t measured(void (*action)(void))
{
    static _Alignas(16) unsigned char stack[HOST_STACK];
    ucontext_t caller, task;
    memset(stack, 0xa5, sizeof(stack));
    assert(getcontext(&task) == 0);
    task.uc_link = &caller;
    task.uc_stack.ss_sp = stack;
    task.uc_stack.ss_size = sizeof(stack);
    makecontext(&task, action, 0);
    assert(swapcontext(&caller, &task) == 0);
    size_t i = 0;
    while (i < sizeof(stack) && stack[i] == 0xa5)
        i++;
    assert(i > 0);
    return sizeof(stack) - i;
}
static double milliseconds(void)
{
    struct timespec now;
    assert(clock_gettime(CLOCK_MONOTONIC, &now) == 0);
    return now.tv_sec * 1000.0 + now.tv_nsec / 1e6;
}
static int compare(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
int main(int argc, char **argv)
{
    assert(argc >= 4);
    graphics = argv[1];
    width = atoi(argv[2]);
    assert(width == 800 || width == 1280);
    height = 1024000 / width;
    lv_init();
    display = lv_display_create(width, height);
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    size_t partial = width * 64 * sizeof(*pixels);
    void *draw = malloc(partial);
    pixels = calloc(3 * width * height, sizeof(*pixels));
    assert(draw && pixels);
    lv_display_set_buffers(display, draw, NULL, partial, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    app_scene_set_reader(demo_read, NULL);
    for (int arg = 3; arg < argc; arg++)
    {
        project = read_json(argv[arg]);
        char error[SM_ERROR_SIZE];
        assert(sm_project_validate(project, error));
        revision++;
        cJSON *pages = cJSON_GetObjectItem(project, "pages");
        for (int i = 0; i < cJSON_GetArraySize(pages); i++)
        {
            atomic_store(&s_page, i);
            double times[FRAMES];
            size_t max_main = 0, max_lvgl = 0;
            for (frame = 0; frame <= FRAMES; frame++)
            {
                connected = frame != 3;
                raw = (frame * 13) % 101;
                unsigned before = flushes;
                size_t main_stack = measured(update);
                size_t lvgl_stack = measured(render);
                if (frame)
                    times[frame - 1] = frame_ms;
                if (main_stack > max_main)
                    max_main = main_stack;
                if (lvgl_stack > max_lvgl)
                    max_lvgl = lvgl_stack;
                assert(s_drawn_page == i && s_runtime_count && !app_scene_diagnostics_visible());
                bool checked = false;
                for (unsigned n = 0; n < s_runtime_count; n++)
                {
                    if (s_runtime[n].value_label)
                    {
                        char expected[32];
                        assert(sm_value_text((sm_signal_t){.valid = connected, .value = raw},
                                             0,
                                             "%",
                                             expected,
                                             sizeof(expected)));
                        assert(!strcmp(lv_label_get_text(s_runtime[n].value_label), expected));
                        checked = true;
                    }
                }
                assert(checked && flushes > before);
            }
            cJSON *elements = cJSON_GetObjectItem(cJSON_GetArrayItem(pages, i), "elements");
            assert(s_runtime_count == (unsigned)cJSON_GetArraySize(elements));
            for (unsigned n = 0; n < s_runtime_count; n++)
            {
                cJSON *el = cJSON_GetArrayItem(elements, n);
                if (!strcmp(sm_json_string(el, "type"), "equipment") &&
                    !cJSON_HasObjectItem(el, "manifold"))
                    assert(s_pictures[n].bytes);
            }
            assert(max_main <= 10240 && max_lvgl <= 32768 && peak <= 32 * 1024 * 1024);
            qsort(times, FRAMES, sizeof(*times), compare);
            printf("%s,%d,%u,%d,%.2f,%zu,%zu,%zu\n",
                   argv[arg],
                   i + 1,
                   s_runtime_count,
                   width,
                   times[18],
                   max_main,
                   max_lvgl,
                   peak);
        }
        cJSON_Delete(project);
    }
    return 0;
}
