execute_process(COMMAND ${PYTHON} "${CMAKE_CURRENT_LIST_DIR}/../tools/build_assets.py"
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/.." COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND ${PYTHON} "${CMAKE_CURRENT_LIST_DIR}/../tools/embed_web.py" "${CMAKE_CURRENT_BINARY_DIR}/web_bundle.inc"
    COMMAND_ERROR_IS_FATAL ANY)
file(GLOB WEB_SOURCES "${CMAKE_CURRENT_LIST_DIR}/../web/*.*")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${WEB_SOURCES}
    "${CMAKE_CURRENT_LIST_DIR}/../tools/embed_web.py" "${CMAKE_CURRENT_LIST_DIR}/../tools/build_assets.py"
    "${CMAKE_CURRENT_LIST_DIR}/../catalog/elements.py")

idf_component_register(
    SRCS "app_main.c" "app_display.c" "app_diagnostics.c" "app_store.c" "app_network.c" "app_web.c" "app_scene.c" "fonts/app_font_20.c"
    INCLUDE_DIRS "." "${CMAKE_CURRENT_BINARY_DIR}"
    REQUIRES nvs_flash esp_timer esp_app_format esp_hw_support esp_system schema_model esp_http_server esp_netif esp_event esp_wifi mdns mbedtls
)

# A BSP alapból belső RAM-ba tenne rajzpuffert; a korábbi bevált burkoló
# a PSRAM-os, CPU-val forgatott útvonalat választja.
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,--wrap=esp_lv_adapter_register_display"
    "-Wl,-u,__wrap_esp_lv_adapter_register_display"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Werror=frame-larger-than=8192)
