set(DISTORTION_SOURCES
    distortion.cpp
    distortion_plugin.cpp
)

add_library(distortion_plugin SHARED ${DISTORTION_SOURCES})

target_include_directories(distortion_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(distortion_plugin
    PRIVATE
        openamp-dsp
)
