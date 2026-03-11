set(WAH_SOURCES
    wah.cpp
    wah_plugin.cpp
)

add_library(wah_plugin SHARED ${WAH_SOURCES})

target_include_directories(wah_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wah_plugin
    PRIVATE
        openamp-dsp
)
