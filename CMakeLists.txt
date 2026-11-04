cmake_minimum_required(VERSION 3.16)

# Az ESP-IDF 5.5.4 komponensrendszere építi a P4 platformalapot.
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
if(NOT EXISTS "${CMAKE_CURRENT_LIST_DIR}/version.txt")
    set(PROJECT_VER "0.1.0-ms1")
endif()
project(zolee_schema_monitor)

include("${CMAKE_CURRENT_LIST_DIR}/cmake/display_psram.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/cmake/hosted_rx.cmake")
