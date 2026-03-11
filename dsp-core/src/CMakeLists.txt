set(DSP_SOURCES
    amp_simulator.cpp
    dsp_engine.cpp
    effect_chain.cpp
    input_processor.cpp
    plugin_loader.cpp
    preset_store.cpp
    ir_loader.cpp
    # Include built-in effects directly
    ${CMAKE_CURRENT_LIST_DIR}/../plugins/noise_gate/noise_gate.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../plugins/compressor/compressor.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../plugins/eq/eq.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../plugins/distortion/distortion.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../plugins/delay/delay.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../plugins/reverb/reverb.cpp
)

add_library(openamp-dsp STATIC ${DSP_SOURCES})

target_include_directories(openamp-dsp
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/../include
        ${CMAKE_CURRENT_LIST_DIR}/../plugins/noise_gate
        ${CMAKE_CURRENT_LIST_DIR}/../plugins/compressor
        ${CMAKE_CURRENT_LIST_DIR}/../plugins/eq
        ${CMAKE_CURRENT_LIST_DIR}/../plugins/distortion
        ${CMAKE_CURRENT_LIST_DIR}/../plugins/delay
        ${CMAKE_CURRENT_LIST_DIR}/../plugins/reverb
)

if(ANDROID)
    target_link_libraries(openamp-dsp log)
endif()
