# InterfacePico
# A ZX Spectrum Interface based on RP2040 PICO30

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 20)

option(ENABLE_USB_STDIO "Enable USB debugging" ON)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(InterfacePico)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)
set(PICO_CXX_ENABLE_RTTI 1)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

include(FetchContent)

# # fetch https://github.com/bog-dan-ro/RP2040_SdFat.git
# FetchContent_Declare(
#   SdFat
#   GIT_REPOSITORY https://github.com/bog-dan-ro/RP2040_SdFat.git
#   GIT_TAG        master
# )
# FetchContent_MakeAvailable(SdFat)


add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
)

pico_generate_pio_header(${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/zx.pio)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    NDEBUG
    PICO_STACK_SIZE=0x1000
    PICO_CORE1_STACK_SIZE=0x1000
    PICO_HEAP_SIZE=0x20000

    # SD card
    SD_MOSI=19
    SD_MISO=16
    SD_SCK=18
    SD_SS=17

)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${CMAKE_PROJECT_NAME}
    # SdFat
    hardware_pio
    hardware_adc
    pico_stdlib
    pico_multicore
)

target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE -Werror -Wall)
if (ENABLE_USB_STDIO)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_USB_STDIO)
    # enable usb output, disable uart output
    pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
    pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)
endif()


# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${CMAKE_PROJECT_NAME})
