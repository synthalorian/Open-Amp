set(EQ_SOURCES
    eq.cpp
    eq_plugin.cpp
)

add_library(eq_plugin SHARED ${EQ_SOURCES})

target_include_directories(eq_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(eq_plugin
    PRIVATE
        openamp-dsp
)
