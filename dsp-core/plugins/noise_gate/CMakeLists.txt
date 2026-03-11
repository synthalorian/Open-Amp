set(NOISE_GATE_SOURCES
    noise_gate.cpp
    noise_gate_plugin.cpp
)

add_library(noise_gate_plugin SHARED ${NOISE_GATE_SOURCES})

target_include_directories(noise_gate_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(noise_gate_plugin
    PRIVATE
        openamp-dsp
)
