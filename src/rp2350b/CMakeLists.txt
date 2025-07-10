# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion RISCV_RPI_2_0_0_5)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD gen4_rp2350_28 CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(IFp C CXX ASM)

set(PICO_CXX_ENABLE_EXCEPTIONS 1)

set(PICO_CXX_ENABLE_RTTI 1)

option(ENABLE_USB_STDIO "Enable USB debugging" ON)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(${CMAKE_PROJECT_NAME}
    main.cpp
    48.rom.cpp
    testrom.bin.cpp
    utils.cpp
    utils.h
    zx.cpp
    zx.h
    zx.pio
)

pico_set_program_name(${CMAKE_PROJECT_NAME} "${CMAKE_PROJECT_NAME}")
pico_set_program_version(${CMAKE_PROJECT_NAME} "0.1")

# Generate PIO header
pico_generate_pio_header(${CMAKE_PROJECT_NAME} ${CMAKE_CURRENT_LIST_DIR}/zx.pio)

if (ENABLE_USB_STDIO)
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE ENABLE_USB_STDIO)
    # enable usb output, disable uart output
    pico_enable_stdio_usb(${CMAKE_PROJECT_NAME} 1)
    pico_enable_stdio_uart(${CMAKE_PROJECT_NAME} 0)
endif()

# Add the standard library to the build
target_link_libraries(${CMAKE_PROJECT_NAME}
        pico_stdlib
        pico_multicore
)

# Add the standard include files to the build
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
        hardware_uart
        hardware_spi
        hardware_pio
)

pico_add_extra_outputs(${CMAKE_PROJECT_NAME})
