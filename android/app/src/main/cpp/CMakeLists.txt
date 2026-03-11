cmake_minimum_required(VERSION 3.20)
project(OpenAmpAndroid LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DSP_CORE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../../../dsp-core")
set(DSP_CORE_SRC_DIR "${DSP_CORE_DIR}/src")

add_subdirectory("${DSP_CORE_SRC_DIR}" "${CMAKE_BINARY_DIR}/dsp-core")

add_library(openamp-native SHARED
    AudioEngine.cpp
    JniBridge.cpp
    ${DSP_CORE_DIR}/plugins/delay/delay.cpp
    ${DSP_CORE_DIR}/plugins/distortion/distortion.cpp
    ${DSP_CORE_DIR}/plugins/reverb/reverb.cpp
    ${DSP_CORE_DIR}/plugins/noise_gate/noise_gate.cpp
    ${DSP_CORE_DIR}/plugins/cabinet/cabinet.cpp
    ${DSP_CORE_DIR}/plugins/acoustic_sim/acoustic_sim.cpp
    ${DSP_CORE_DIR}/plugins/harmonizer/harmonizer.cpp
    ${DSP_CORE_SRC_DIR}/ir_loader.cpp
)

target_include_directories(openamp-native
    PRIVATE
        ${DSP_CORE_DIR}/include
        ${DSP_CORE_DIR}/plugins/delay
        ${DSP_CORE_DIR}/plugins/distortion
        ${DSP_CORE_DIR}/plugins/reverb
        ${DSP_CORE_DIR}/plugins/noise_gate
        ${DSP_CORE_DIR}/plugins/cabinet
        ${DSP_CORE_DIR}/plugins/acoustic_sim
        ${DSP_CORE_DIR}/plugins/harmonizer
        ${CMAKE_CURRENT_LIST_DIR}
)

if(DEFINED OBOE_DIR)
    add_subdirectory(${OBOE_DIR} "${CMAKE_BINARY_DIR}/oboe")
    target_link_libraries(openamp-native PRIVATE oboe)
else()
    message(FATAL_ERROR "OBOE_DIR not set. Provide the path to Oboe.")
endif()

find_library(log-lib log)
target_link_libraries(openamp-native PRIVATE openamp-dsp ${log-lib})
