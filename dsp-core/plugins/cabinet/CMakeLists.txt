add_library(cabinet_plugin SHARED
    cabinet.cpp
    cabinet_plugin.cpp
)

target_include_directories(cabinet_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(cabinet_plugin PRIVATE
    openamp-dsp
)

set_target_properties(cabinet_plugin PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "cabinet_plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dsp-core/plugins/cabinet"
)
