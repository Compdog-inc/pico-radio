cmake_minimum_required(VERSION 3.30)

if(NOT PICO_BOARD)
    set(PICO_BOARD pico_w)
endif()

# Pull in SDK (must be before project)
include("$ENV{PICO_SDK_PATH}\\external\\pico_sdk_import.cmake")
include("$ENV{FREERTOS_KERNEL_PATH}\\portable\\ThirdParty\\GCC\\RP2040\\FreeRTOS_Kernel_import.cmake")

project(pico-radio C CXX ASM)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)

if(NOT PICO_CYW43_SUPPORTED)
    message(FATAL_ERROR "The pico-radio library requires CYW43 support!")
endif()

if (PICO_SDK_VERSION_STRING VERSION_LESS "2.0.0")
        message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.0.0 (or later) required. Your version is ${PICO_SDK_VERSION_STRING}")
endif()

if(NOT PICO_RADIO_IP_MASKED)
        set(PICO_RADIO_IP_MASKED 10,67,31)
endif()

if(NOT PICO_RADIO_AP)
        set(PICO_RADIO_AP 0)
else()
        set(PICO_RADIO_AP 1)
endif()

if(NOT PICO_RADIO_HOSTNAME)
        set(PICO_RADIO_HOSTNAME "Pico-Radio")
endif()

if(NOT PICO_RADIO_SSID)
        set(PICO_RADIO_SSID "PicoWifi")
endif()

if(NOT PICO_RADIO_PASSWORD)
        set(PICO_RADIO_OPEN 1)
else()
        set(PICO_RADIO_OPEN 0)
endif()

if(NOT WEBSOCKET_THREAD_STACK_SIZE)
        set(WEBSOCKET_THREAD_STACK_SIZE 4096)
endif()

if(NOT WEBSOCKET_TIMEOUT)
        set(WEBSOCKET_TIMEOUT 5000)
endif()

message("Radio hostname is '${PICO_RADIO_HOSTNAME}'.")

if(PICO_RADIO_OPEN)
        if(PICO_RADIO_AP)
                message("Flashing radio with open wifi '${PICO_RADIO_SSID}' in AP mode.")
        else()
                message("Flashing radio with open wifi '${PICO_RADIO_SSID}' in STA mode.")
        endif()
else()
        if(PICO_RADIO_AP)
                message("Flashing radio with encrypted wifi '${PICO_RADIO_SSID}' / '${PICO_RADIO_PASSWORD}' in AP mode.")
        else()
                message("Flashing radio with encrypted wifi '${PICO_RADIO_SSID}' / '${PICO_RADIO_PASSWORD}' in STA mode.")
        endif()
endif()

# Initialize the SDK
pico_sdk_init()

add_compile_options(-Wall
        -Wno-format          # int != int32_t as far as the compiler is concerned because gcc has int32_t as long int
        -Wno-unused-function # we have some for the docs that aren't called
        -Wno-maybe-uninitialized
        )

configure_file( config.h.in ${CMAKE_BINARY_DIR}/generated/pico-radio/config.h )
include_directories( ${CMAKE_BINARY_DIR}/generated/pico-radio/ ) 

add_library(pico-radio STATIC
        src/radio.cpp
        src/dhcpserver.c
        src/tcplistener.cpp
        src/tcpclient.cpp
        src/textstream.cpp
        src/udpsocket.cpp
        src/guid.cpp
        src/websocket.cpp
        src/wsserver.cpp
        )

target_include_directories(pico-radio PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include
        ${CMAKE_CURRENT_LIST_DIR}/..
        ${CMAKE_CURRENT_LIST_DIR}/
        )
target_link_libraries(pico-radio
        pico_cyw43_arch_lwip_sys_freertos
        pico_stdlib
        pico_rand
        FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
        )