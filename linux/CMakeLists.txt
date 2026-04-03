cmake_minimum_required(VERSION 3.20)
project(OpenAmp VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Options
option(USE_PIPEWIRE "Use PipeWire audio backend" ON)
option(USE_ALSA "Include ALSA fallback backend" ON)
option(BUILD_TESTS "Build unit tests" OFF)

# Find packages
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Quick QuickControls2)
find_package(PkgConfig REQUIRED)

# JACK (required for real-time guitar processing)
pkg_check_modules(JACK jack)
if(JACK_FOUND)
    message(STATUS "JACK found: ${JACK_VERSION}")
    add_definitions(-DUSE_JACK)
else()
    message(FATAL_ERROR "JACK not found. Install: sudo pacman -S pipewire-jack")
endif()

# PipeWire
if(USE_PIPEWIRE)
    pkg_check_modules(PIPEWIRE libpipewire-0.3)
    if(PIPEWIRE_FOUND)
        message(STATUS "PipeWire found: ${PIPEWIRE_VERSION}")
        add_definitions(-DUSE_PIPEWIRE)
    else()
        message(WARNING "PipeWire not found, skipping PipeWire backend")
        set(USE_PIPEWIRE OFF)
    endif()
endif()

# ALSA
if(USE_ALSA)
    pkg_check_modules(ALSA alsa)
    if(ALSA_FOUND)
        message(STATUS "ALSA found: ${ALSA_VERSION}")
        add_definitions(-DUSE_ALSA)
    else()
        message(WARNING "ALSA not found")
    endif()
endif()

# DSP Core
set(DSP_CORE_DIR ${CMAKE_SOURCE_DIR}/../dsp-core)
add_subdirectory(${DSP_CORE_DIR} ${CMAKE_BINARY_DIR}/dsp-core)

# Sources
set(SOURCES
    src/main.cpp
    src/audio/audio_backend.cpp
    src/audio/pipewire_backend.cpp
    src/audio/pipewire_direct_backend.cpp
    src/audio/alsa_backend.cpp
    src/audio/jack_backend.cpp
    src/audio/midi_handler.cpp
    src/ui/main_window.cpp
    src/ui/knob_widget.cpp
    src/ui/pedal_widget.cpp
    src/ui/preset_browser.cpp
    src/ui/effects_panel.cpp
    src/ui/amp_panel.cpp
    src/ui/settings_dialog.cpp
    src/ui/theme.cpp
    src/ui/meter_widget.cpp
    src/ui/ir_panel_widget.cpp
    src/ui/latency_display_widget.cpp
)

# Headers
set(HEADERS
    src/audio/audio_backend.h
    src/audio/pipewire_backend.h
    src/audio/pipewire_direct_backend.h
    src/audio/alsa_backend.h
    src/audio/jack_backend.h
    src/audio/midi_handler.h
    src/ui/main_window.h
    src/ui/knob_widget.h
    src/ui/pedal_widget.h
    src/ui/preset_browser.h
    src/ui/effects_panel.h
    src/ui/amp_panel.h
    src/ui/settings_dialog.h
    src/ui/theme.h
    src/ui/meter_widget.h
    src/ui/ir_panel_widget.h
    src/ui/latency_display_widget.h
)

# Executable
add_executable(openamp ${SOURCES} ${HEADERS})

target_include_directories(openamp PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${DSP_CORE_DIR}/include
    ${DSP_CORE_DIR}
)

target_link_libraries(openamp PRIVATE
    openamp-dsp
    looper_plugin
    metronome_plugin
    tuner_plugin
    Qt6::Core
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
)

if(PIPEWIRE_FOUND)
    target_include_directories(openamp PRIVATE ${PIPEWIRE_INCLUDE_DIRS})
    target_link_libraries(openamp PRIVATE ${PIPEWIRE_LIBRARIES})
endif()

if(ALSA_FOUND)
    target_include_directories(openamp PRIVATE ${ALSA_INCLUDE_DIRS})
    target_link_libraries(openamp PRIVATE ${ALSA_LIBRARIES})
endif()

if(JACK_FOUND)
    target_include_directories(openamp PRIVATE ${JACK_INCLUDE_DIRS})
    target_link_libraries(openamp PRIVATE ${JACK_LIBRARIES})
endif()

# Install
install(TARGETS openamp RUNTIME DESTINATION bin)
install(DIRECTORY resources/icons DESTINATION share/openamp)
install(FILES resources/openamp.desktop DESTINATION share/applications)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/../presets/factory DESTINATION share/openamp/presets)

# Tests
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
