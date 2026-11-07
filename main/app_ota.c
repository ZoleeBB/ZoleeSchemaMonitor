#include "app_ota.h"
#include "app_assets.h"
#include "esp_app_desc.h"
#include "esp_app_format.h"
#include "esp_ota_ops.h"
#include "ota_identity.inc"
#include <stdatomic.h>
#include <stdlib.h>
#include <string.h>

/* Az IDF linker közvetlenül az esp_app_desc után helyezi el ezt a leírót.
 * A csomagoló is ellenőrzi az offsetet a tényleges binárisban. */
const char schema_ota_identity[] __attribute__((section(".rodata_custom_desc"), used, aligned(1))) =
    SCHEMA_OTA_IDENTITY;
static atomic_flag s_upload = ATOMIC_FLAG_INIT;
static bool s_boot_checked;
static uint64_t s_next_asset_check;

static esp_err_t
upload(size_t size, app_ota_read_fn reader, void *context, bool (*verify)(void *context))
{
    const esp_partition_t *target = esp_ota_get_next_update_partition(NULL);
    size_t prefix_size = sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) +
                         sizeof(esp_app_desc_t) + sizeof(schema_ota_identity);
    if (!reader || !target || size < prefix_size || size > target->size || prefix_size > 4096)
        return ESP_ERR_INVALID_SIZE;
    unsigned char *buffer = malloc(4096);
    if (!buffer)
        return ESP_ERR_NO_MEM;
    size_t used = 0;
    while (used < prefix_size)
    {
        int count = reader(context, buffer + used, prefix_size - used);
        if (count <= 0 || (size_t)count > prefix_size - used)
        {
            free(buffer);
            return ESP_FAIL;
        }
        used += count;
    }
    esp_image_header_t image;
    esp_app_desc_t descriptor;
    memcpy(&image, buffer, sizeof(image));
    memcpy(&descriptor,
           buffer + sizeof(image) + sizeof(esp_image_segment_header_t),
           sizeof(descriptor));
    const esp_app_desc_t *running = esp_app_get_description();
    char candidate[APP_ASSET_ID_SIZE];
    memcpy(candidate, buffer + prefix_size - sizeof(schema_ota_identity), sizeof(candidate));
    bool valid =
        image.magic == ESP_IMAGE_HEADER_MAGIC && image.hash_appended == 1 &&
        descriptor.magic_word == ESP_APP_DESC_MAGIC_WORD &&
        !memcmp(descriptor.project_name, running->project_name, sizeof(descriptor.project_name)) &&
        candidate[sizeof(candidate) - 1] == 0 && app_assets_identity_valid(candidate) &&
        app_assets_ready(candidate);
    if (!valid)
    {
        free(buffer);
        return ESP_ERR_INVALID_ARG;
    }
    esp_ota_handle_t handle = 0;
    esp_err_t result = esp_ota_begin(target, size, &handle);
    if (result != ESP_OK)
    {
        free(buffer);
        return result;
    }
    result = esp_ota_write(handle, buffer, used);
    while (result == ESP_OK && used < size)
    {
        size_t wanted = size - used > 4096 ? 4096 : size - used;
        int count = reader(context, buffer, wanted);
        if (count <= 0 || (size_t)count > wanted)
        {
            result = ESP_FAIL;
            break;
        }
        result = esp_ota_write(handle, buffer, count);
        used += count;
    }
    free(buffer);
    if (result != ESP_OK)
    {
        esp_ota_abort(handle);
        return result;
    }
    /* Az IDF ellenőrzi a teljes image-et, hash-t, célchipet és revíziót.
     * A bootcél csak a teljes feltöltés és sikeres ellenőrzés után változhat. */
    result = esp_ota_end(handle);
    if (result != ESP_OK)
        return result;
    if (verify && !verify(context))
        return ESP_ERR_INVALID_ARG;
    return esp_ota_set_boot_partition(target);
}

esp_err_t app_ota_upload(size_t size, app_ota_read_fn reader, void *context)
{
    return app_ota_upload_verified(size, reader, context, NULL);
}

esp_err_t app_ota_upload_verified(size_t size,
                                  app_ota_read_fn reader,
                                  void *context,
                                  bool (*verify)(void *context))
{
    if (atomic_flag_test_and_set(&s_upload))
        return ESP_ERR_INVALID_STATE;
    esp_err_t result = upload(size, reader, context, verify);
    /* Siker után az újraindulásig nem írható felül a már ellenőrzött bootcél. */
    if (result != ESP_OK)
        atomic_flag_clear(&s_upload);
    return result;
}

void app_ota_boot_poll(bool healthy, uint64_t uptime_seconds)
{
    if (s_boot_checked)
        return;
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (!running || esp_ota_get_state_partition(running, &state) != ESP_OK ||
        state != ESP_OTA_IMG_PENDING_VERIFY)
    {
        s_boot_checked = true;
        return;
    }
    /* A teljes SD-készlet hash-elése csak függő firmware-nél szükséges.
     * A határidőnél a közben helyreállított készlet még egyszer ellenőrizhető. */
    if (healthy && uptime_seconds >= 30 &&
        (uptime_seconds >= s_next_asset_check || uptime_seconds >= 180))
    {
        s_next_asset_check = uptime_seconds + 10;
        if (app_assets_boot_ready() && esp_ota_mark_app_valid_cancel_rollback() == ESP_OK)
            s_boot_checked = true;
    }
    if (!s_boot_checked && uptime_seconds >= 180)
    {
        if (esp_ota_mark_app_invalid_rollback_and_reboot() == ESP_OK)
            s_boot_checked = true;
    }
}
