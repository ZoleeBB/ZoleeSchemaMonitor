idf_component_register(
    SRCS "schema_model.c" "schema_catalog.c" "schema_journal.c"
    INCLUDE_DIRS "include"
    REQUIRES json schema_core
)
