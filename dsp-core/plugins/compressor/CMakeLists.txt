set(COMPRESSOR_SOURCES
    compressor.cpp
    compressor_plugin.cpp
)

add_library(compressor_plugin SHARED ${COMPRESSOR_SOURCES})

target_include_directories(compressor_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(compressor_plugin
    PRIVATE
        openamp-dsp
)
