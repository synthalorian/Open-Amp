cmake_minimum_required(VERSION 3.20)

# Fetch Catch2
include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.0
)
FetchContent_MakeAvailable(Catch2)

# Test sources
set(TEST_SOURCES
    test_main.cpp
    test_dsp_utils.cpp
    test_amp_simulator.cpp
    test_distortion.cpp
    test_delay.cpp
    test_reverb.cpp
    test_preset_store.cpp
)

add_executable(dsp-unit-tests ${TEST_SOURCES})

target_include_directories(dsp-unit-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/plugins
)

target_link_libraries(dsp-unit-tests PRIVATE
    openamp-dsp
    Catch2::Catch2WithMain
)

target_compile_features(dsp-unit-tests PRIVATE cxx_std_17)

enable_testing()
add_test(NAME DSPUnitTests COMMAND dsp-unit-tests)
