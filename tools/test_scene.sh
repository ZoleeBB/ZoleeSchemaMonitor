#!/usr/bin/env bash
# Az éles rajzoló valódi LVGL objektumokkal, gazdagépen is ellenőrizhető.
set -euo pipefail
: "${LVGL_HOST_BUILD:?A gazdagépre fordított LVGL buildkönyvtára szükséges}"
scene_lvgl="${LVGL_SOURCE:-managed_components/lvgl__lvgl}"
scene_tls="${MBEDTLS_PATH:-${IDF_PATH:-}/components/mbedtls/mbedtls}"
scene_json="${CJSON_PATH:-${IDF_PATH:-}/components/json/cJSON}"
scene_binary="$(mktemp /tmp/schema-scene-test-XXXXXX)"
trap 'rm -f "$scene_binary"' EXIT
gcc -std=c11 -Wall -Wextra -Werror -DLV_CONF_SKIP=1 -DLV_USE_STDLIB_MALLOC=LV_STDLIB_CLIB -DSCHEMA_SCENE_TEST -DMBEDTLS_SHA256_C '-DMBEDTLS_CONFIG_FILE="native_mbedtls_config.h"' \
    -Itests/native/stubs -I"$scene_lvgl" -I"$scene_tls/include" -I"$scene_json" -Imain -Icomponents/schema_core/include -Icomponents/schema_model/include \
    tests/native/scene_test.c main/fonts/app_font_20.c components/schema_core/schema_core.c components/schema_model/schema_model.c components/schema_model/schema_catalog.c \
    "$scene_json/cJSON.c" "$scene_tls/library/sha256.c" "$scene_tls/library/platform_util.c" \
    "$LVGL_HOST_BUILD/lib/liblvgl.a" -lm -o "$scene_binary"
"$scene_binary"
