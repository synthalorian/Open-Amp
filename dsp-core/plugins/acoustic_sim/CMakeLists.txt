add_library(acoustic_sim_plugin SHARED
    acoustic_sim.cpp
    acoustic_sim_plugin.cpp
)

target_include_directories(acoustic_sim_plugin PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_link_libraries(acoustic_sim_plugin PRIVATE
    openamp-dsp
)

set_target_properties(acoustic_sim_plugin PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "acoustic_sim_plugin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/dsp-core/plugins/acoustic_sim"
)
