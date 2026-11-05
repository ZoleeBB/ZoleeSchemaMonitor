#pragma once
#include <stdint.h>
#define ESP_IMAGE_HEADER_MAGIC 0xe9
typedef struct
{
    uint8_t magic;
    uint8_t reserved[22];
    uint8_t hash_appended;
} esp_image_header_t;
typedef struct
{
    uint32_t load_addr, data_len;
} esp_image_segment_header_t;
