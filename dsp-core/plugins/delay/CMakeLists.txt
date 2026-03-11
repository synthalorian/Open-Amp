set(DELAY_SOURCES
    delay.cpp
    delay_plugin.cpp
)

add_library(delay_plugin SHARED ${DELAY_SOURCES})

target_include_directories(delay_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(delay_plugin
    PRIVATE
        openamp-dsp
)
