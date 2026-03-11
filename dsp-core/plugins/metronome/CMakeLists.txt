set(METRONOME_SOURCES
    metronome.cpp
    metronome_plugin.cpp
)

add_library(metronome_plugin SHARED ${METRONOME_SOURCES})

target_include_directories(metronome_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(metronome_plugin
    PRIVATE
        openamp-dsp
)
