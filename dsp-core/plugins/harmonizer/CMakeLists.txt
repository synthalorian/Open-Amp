add_library(harmonizer_plugin SHARED
    harmonizer.cpp
    harmonizer_plugin.cpp
)

target_include_directories(harmonizer_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(harmonizer_plugin PRIVATE
    openamp-dsp
)

set_target_properties(harmonizer_plugin PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "harmonizer_plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dsp-core/plugins/harmonizer"
)
