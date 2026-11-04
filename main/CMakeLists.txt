idf_component_register(
    SRCS "app_main.c" "app_display.c" "app_diagnostics.c"
    INCLUDE_DIRS "."
    REQUIRES nvs_flash esp_timer esp_app_format esp_hw_support esp_system schema_core
)

# A BSP alapból belső RAM-ba tenne rajzpuffert; a korábbi bevált burkoló
# a PSRAM-os, CPU-val forgatott útvonalat választja.
target_link_options(${COMPONENT_LIB} INTERFACE
    "-Wl,--wrap=esp_lv_adapter_register_display"
    "-Wl,-u,__wrap_esp_lv_adapter_register_display"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Werror=frame-larger-than=8192)
