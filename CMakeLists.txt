cmake_minimum_required(VERSION 3.23)

include("pico-sdk/pico_sdk_init.cmake")
project(pico_webserver LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 23)

pico_sdk_init()

file(GLOB fs_files CONFIGURE_DEPENDS "fs/*")
add_custom_command(OUTPUT "include/fsdata.h"
    DEPENDS ${fs_files}
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}"
    COMMAND perl "pico-sdk/lib/lwip/src/apps/http/makefsdata/makefsdata"
    COMMAND mv "fsdata.c" "include/fsdata.h")
add_custom_target(fsdata DEPENDS "include/fsdata.h")

add_library(lwipopts INTERFACE)
target_include_directories(lwipopts INTERFACE
    "include/lwip")
add_dependencies(lwipopts fsdata)

add_library(tusb_config INTERFACE)
target_include_directories(tusb_config INTERFACE
    "include/tusb")

# Extra stuff from TinyUSB, that is not part of tinyusb_device library
add_library(tinyusb_lib_networking STATIC
    "${PICO_TINYUSB_PATH}/lib/networking/dhserver.c"
    "${PICO_TINYUSB_PATH}/lib/networking/rndis_reports.c")
target_include_directories(tinyusb_lib_networking PUBLIC
    "${PICO_TINYUSB_PATH}/lib/networking")
target_link_libraries(tinyusb_lib_networking PUBLIC
    lwipopts
    tusb_config
    pico_stdlib
    tinyusb_device
    pico_lwip
    pico_lwip_http)

add_executable(pico_webserver)
target_sources(pico_webserver PRIVATE
    "include/encre_file.h"
    "src/encre_file.c"
    "include/GDEP073E01.h"
    "src/GDEP073E01.c"
    "src/tusb_lwip_glue.c"
    "src/usb_descriptors.c"
    "src/webserver.c")
target_compile_definitions(pico_webserver PRIVATE PICO_ENTER_USB_BOOT_ON_EXIT=1)
target_include_directories(pico_webserver PRIVATE "include")
target_link_libraries(pico_webserver PRIVATE
    pico_unique_id
    pico_multicore
    hardware_spi
    tinyusb_lib_networking)

pico_enable_stdio_usb(pico_webserver 0)
pico_enable_stdio_uart(pico_webserver 0)
pico_add_extra_outputs(pico_webserver)
