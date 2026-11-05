execute_process(COMMAND ${PYTHON} "${CMAKE_CURRENT_LIST_DIR}/../tools/ota_identity.py" "${CMAKE_CURRENT_BINARY_DIR}/ota_identity.inc"
    COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${PYTHON} "${CMAKE_CURRENT_LIST_DIR}/../tools/build_assets.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.." COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${PYTHON} "${CMAKE_CURRENT_LIST_DIR}/../tools/embed_web.py" "${CMAKE_CURRENT_BINARY_DIR}/web_bundle.inc"
    COMMAND_ERROR_IS_FATAL ANY)
file(GLOB_RECURSE ELEMENT_IMAGES "${CMAKE_CURRENT_LIST_DIR}/../assets/elements/*.png")
file(GLOB WEB_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../web/*.*")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WEB_SOURCES} ${ELEMENT_IMAGES}
    "${CMAKE_CURRENT_LIST_DIR}/../tools/requirements-assets.txt"
    "${CMAKE_CURRENT_LIST_DIR}/../tools/embed_web.py" "${CMAKE_CURRENT_LIST_DIR}/../tools/build_assets.py"
    "${CMAKE_CURRENT_LIST_DIR}/../catalog/elements.py" "${CMAKE_CURRENT_LIST_DIR}/../tools/ota_identity.py")

idf_component_register(
    SRCS
        "app_main.c"
        "app_display.c"
        "app_diagnostics.c"
        "app_diagnostics_format.c"
        "app_diagnostic_view.c"
        "app_events.c"
        "app_store.c"
        "app_backup.c"
        "app_network.c"
        "app_web.c"
        "app_ota.c"
        "app_assets.c"
        "app_release.c"
        "app_scene.c"
        "app_loxone.c"
        "fonts/app_font_20.c"
    INCLUDE_DIRS "." "${CMAKE_CURRENT_BINARY_DIR}"
    REQUIRES fatfs app_update nvs_flash esp_timer esp_app_format esp_hw_support esp_system schema_model esp_http_server esp_netif esp_event esp_wifi mdns mbedtls esp_http_client esp_websocket_client
)

# A BSP alapból belső RAM-ba tenne rajzpuffert; a korábbi bevált burkoló
# a PSRAM-os, CPU-val forgatott útvonalat választja.
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,-u,schema_ota_identity"
    "-Wl,--wrap=esp_lv_adapter_register_display"
    "-Wl,-u,__wrap_esp_lv_adapter_register_display"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Werror=frame-larger-than=8192)
