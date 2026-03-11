set(LOOPER_SOURCES
    looper.cpp
    looper_plugin.cpp
)

add_library(looper_plugin SHARED ${LOOPER_SOURCES})

target_include_directories(looper_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(looper_plugin
    PRIVATE
        openamp-dsp
)
