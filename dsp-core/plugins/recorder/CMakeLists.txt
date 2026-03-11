set(RECORDER_SOURCES
    recorder.cpp
    recorder_plugin.cpp
)

add_library(recorder_plugin SHARED ${RECORDER_SOURCES})

target_include_directories(recorder_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(recorder_plugin
    PRIVATE
        openamp-dsp
)
