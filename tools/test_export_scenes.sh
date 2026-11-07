#!/usr/bin/env bash
# A gazdagépes LVGL-t az alábbi LV-definíciókkal kell fordítani.
set -euo pipefail
: "${LVGL_HOST_BUILD:?}" "${LVGL_SOURCE:?}" "${MBEDTLS_PATH:?}" "${CJSON_PATH:?}"
binary=$(mktemp)
trap 'rm -f "$binary"' EXIT
gcc -O2 -std=c11 -Wall -Wextra -Werror -DLV_CONF_SKIP=1 -DLV_USE_STDLIB_MALLOC=LV_STDLIB_CLIB \
    -DLV_USE_LODEPNG=1 -DLV_CACHE_DEF_SIZE=4194304 -DSCHEMA_SCENE_TEST -DMBEDTLS_SHA256_C \
    '-DMBEDTLS_CONFIG_FILE="native_mbedtls_config.h"' -Itests/native/stubs -I"$LVGL_SOURCE" \
    -I"$MBEDTLS_PATH/include" -I"$CJSON_PATH" -Imain -Icomponents/schema_core/include \
    -Icomponents/schema_model/include tests/native/export_scene.c main/fonts/app_font_20.c \
    components/schema_core/schema_core.c components/schema_model/{schema_model,schema_catalog}.c \
    "$CJSON_PATH/cJSON.c" "$MBEDTLS_PATH/library/"{sha256,platform_util}.c "$LVGL_HOST_BUILD/lib/liblvgl.a" \
    -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=aligned_alloc,--wrap=free -lm -o "$binary"
for width in 1280 800; do
    LD_BIND_NOW=1 "$binary" "${1:?PNG-csomag könyvtára}" "$width" tests/schemas/*/projekt.json
done
