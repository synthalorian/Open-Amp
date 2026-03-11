set(TUNER_SOURCES
    tuner.cpp
    tuner_plugin.cpp
)

add_library(tuner_plugin SHARED ${TUNER_SOURCES})

target_include_directories(tuner_plugin
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(tuner_plugin
    PRIVATE
        openamp-dsp
)
