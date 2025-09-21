# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico_w CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(atividade-02 C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1

add_executable(atividade-02 
main.c
lib/aht10/aht10.c
lib/aht10/aht10.c
lib/pico_ssd1306/ssd1306.c
)

pico_set_program_name(atividade-02 "atividade-02")
pico_set_program_version(atividade-02 "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(atividade-02 0)
pico_enable_stdio_usb(atividade-02 1)

# Add the standard library to the build
target_link_libraries(atividade-02
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(atividade-02 PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/lib/aht10
        ${CMAKE_CURRENT_LIST_DIR}/lib/pico_ssd1306
)

# Add any user requested libraries
target_link_libraries(atividade-02 
        hardware_i2c
        
        )

pico_add_extra_outputs(atividade-02)

