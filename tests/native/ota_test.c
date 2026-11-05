/* A tényleges OTA-kezelőt SDK-helyettesítőkkel, hibainjektálással vizsgáljuk. */
#include "../../main/app_ota.c"
#include <assert.h>
#include <stdio.h>

_Static_assert(sizeof(esp_app_desc_t) == 256, "app descriptor layout");
_Static_assert(sizeof(esp_image_header_t) == 24, "image header layout");
static const esp_partition_t partition = {.size = 8 * 1024 * 1024};
static const esp_app_desc_t running = {.magic_word = ESP_APP_DESC_MAGIC_WORD,
                                       .project_name = "zolee_schema_monitor"};
static unsigned begins, writes, ends, aborts, switches, confirms, rollbacks;
static esp_err_t begin_error, write_error, end_error, switch_error, confirm_error;
static esp_ota_img_states_t boot_state;
static unsigned char image[10000], flashed[10000];
static size_t offset, length, chunk, stop, written;
static bool ready_set, current_ready = true;
static bool reject_digest(void *context)
{
    (void)context;
    return false;
}
static int read_image(void *context, void *buffer, size_t capacity)
{
    assert(context == image);
    if (offset >= stop)
        return -1;
    size_t count = length - offset;
    if (count > capacity)
        count = capacity;
    if (count > chunk)
        count = chunk;
    if (count > stop - offset)
        count = stop - offset;
    memcpy(buffer, image + offset, count);
    offset += count;
    return (int)count;
}
const esp_app_desc_t *esp_app_get_description(void)
{
    return &running;
}
const esp_partition_t *esp_ota_get_next_update_partition(const esp_partition_t *start)
{
    (void)start;
    return &partition;
}
const esp_partition_t *esp_ota_get_running_partition(void)
{
    return &partition;
}
esp_err_t esp_ota_get_state_partition(const esp_partition_t *part, esp_ota_img_states_t *state)
{
    assert(part == &partition);
    *state = boot_state;
    return ESP_OK;
}
esp_err_t esp_ota_begin(const esp_partition_t *part, size_t size, esp_ota_handle_t *handle)
{
    assert(part == &partition && size == length);
    begins++;
    *handle = 1;
    return begin_error;
}
esp_err_t esp_ota_write(esp_ota_handle_t handle, const void *data, size_t size)
{
    assert(handle == 1 && written + size <= length);
    writes++;
    if (write_error)
        return write_error;
    memcpy(flashed + written, data, size);
    written += size;
    return ESP_OK;
}
esp_err_t esp_ota_end(esp_ota_handle_t handle)
{
    assert(handle == 1 && written == length);
    ends++;
    return end_error;
}
esp_err_t esp_ota_abort(esp_ota_handle_t handle)
{
    assert(handle == 1);
    aborts++;
    return ESP_OK;
}
esp_err_t esp_ota_set_boot_partition(const esp_partition_t *part)
{
    assert(part == &partition && ends == 1);
    switches++;
    return switch_error;
}
esp_err_t esp_ota_mark_app_valid_cancel_rollback(void)
{
    confirms++;
    return confirm_error;
}
esp_err_t esp_ota_mark_app_invalid_rollback_and_reboot(void)
{
    rollbacks++;
    return ESP_OK;
}

static void reset(void)
{
    begins = writes = ends = aborts = switches = confirms = rollbacks = 0;
    begin_error = write_error = end_error = switch_error = confirm_error = ESP_OK;
    ready_set = false;
    current_ready = true;
    s_boot_checked = false;
    boot_state = ESP_OTA_IMG_PENDING_VERIFY;
    atomic_flag_clear(&s_upload);
    memset(image, 0x5a, sizeof(image));
    esp_image_header_t header = {.magic = ESP_IMAGE_HEADER_MAGIC, .hash_appended = 1};
    memcpy(image, &header, sizeof(header));
    memcpy(image + 32, &running, sizeof(running));
    memcpy(image + 288, schema_ota_identity, sizeof(schema_ota_identity));
    offset = written = 0;
    length = sizeof(image);
    chunk = 7;
    stop = length;
}
static esp_err_t upload_image(void)
{
    return app_ota_upload(length, read_image, image);
}
int main(void)
{
    reset();
    assert(upload_image() == ESP_OK);
    assert(begins == 1 && ends == 1 && switches == 1 && aborts == 0);
    assert(written == length && !memcmp(image, flashed, length));
    assert(upload_image() == ESP_ERR_INVALID_STATE && begins == 1);
    /* Eltérő projekt, grafika, image magic, leíró és hashjel már törlés előtt elutasul. */
    const size_t corrupt[] = {0, 23, 32, 80, 288};
    for (size_t i = 0; i < sizeof(corrupt) / sizeof(*corrupt); i++)
    {
        reset();
        image[corrupt[i]] ^= 1;
        assert(upload_image() == ESP_ERR_INVALID_ARG && begins == 0 && switches == 0);
    }
    reset();
    assert(app_ota_upload(100, read_image, image) == ESP_ERR_INVALID_SIZE && !begins);
    assert(app_ota_upload(partition.size + 1, read_image, image) == ESP_ERR_INVALID_SIZE &&
           !begins);
    reset();
    stop = 100;
    assert(upload_image() != ESP_OK && !begins);
    reset();
    stop = 600;
    assert(upload_image() != ESP_OK && aborts == 1 && !ends && !switches);
    /* Egy megszakított kísérlet után ugyanazon a booton újrapróbálható. */
    offset = written = 0;
    stop = length;
    assert(upload_image() == ESP_OK);
    reset();
    begin_error = ESP_FAIL;
    assert(upload_image() != ESP_OK && !aborts && !switches);
    reset();
    write_error = ESP_FAIL;
    assert(upload_image() != ESP_OK && aborts == 1 && !ends && !switches);
    reset();
    end_error = ESP_FAIL;
    assert(upload_image() != ESP_OK && ends == 1 && !aborts && !switches);
    reset();
    switch_error = ESP_FAIL;
    assert(upload_image() != ESP_OK && switches == 1);
    reset();
    atomic_flag_test_and_set(&s_upload);
    assert(upload_image() == ESP_ERR_INVALID_STATE && !begins);
    reset();
    app_ota_boot_poll(true, 29);
    assert(!confirms);
    app_ota_boot_poll(true, 30);
    assert(confirms == 1 && !rollbacks);
    app_ota_boot_poll(false, 180);
    assert(!rollbacks);
    reset();
    app_ota_boot_poll(false, 179);
    assert(!rollbacks);
    app_ota_boot_poll(false, 180);
    assert(rollbacks == 1 && !confirms);
    reset();
    confirm_error = ESP_FAIL;
    app_ota_boot_poll(true, 180);
    assert(confirms == 1 && rollbacks == 1);
    reset();
    current_ready = false;
    assert(upload_image() != ESP_OK && !begins);
    reset();
    image[288 + 76] = image[288 + 76] == 'a' ? 'b' : 'a';
    assert(upload_image() != ESP_OK && !begins);
    ready_set = true;
    offset = 0;
    assert(upload_image() == ESP_OK && switches == 1);
    reset();
    assert(app_ota_upload_verified(length, read_image, image, reject_digest) ==
               ESP_ERR_INVALID_ARG &&
           ends == 1 && !switches);
    reset();
    boot_state = ESP_OTA_IMG_VALID;
    app_ota_boot_poll(false, 180);
    assert(!confirms && !rollbacks);
    puts("Native OTA: stream, compatibility, interruptions, flash failures, boot switch and "
         "rollback policy OK");
}

bool app_assets_identity_valid(const char *identity)
{
    return strlen(identity) == APP_ASSET_ID_SIZE - 1 && !strncmp(identity, "ZSM-ASSETS-1:", 13);
}
bool app_assets_ready(const char *identity)
{
    return !strcmp(identity, schema_ota_identity) ? current_ready : ready_set;
}
