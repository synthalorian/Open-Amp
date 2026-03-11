set(REVERB_SOURCES
    reverb.cpp
    reverb_plugin.cpp
)

add_library(reverb_plugin SHARED ${REVERB_SOURCES})

target_include_directories(reverb_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(reverb_plugin
    PRIVATE
        openamp-dsp
)
