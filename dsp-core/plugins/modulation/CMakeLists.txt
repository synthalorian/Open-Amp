set(MODULATION_SOURCES
    modulation.cpp
    modulation_plugin.cpp
)

add_library(modulation_plugin SHARED ${MODULATION_SOURCES})

target_include_directories(modulation_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(modulation_plugin
    PRIVATE
        openamp-dsp
)
