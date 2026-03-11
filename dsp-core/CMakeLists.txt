cmake_minimum_required(VERSION 3.20)
project(OpenAmpDSP VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Build unit tests" OFF)
option(BUILD_PLUGINS "Build effect plugins" ON)

add_subdirectory(src)

if(BUILD_PLUGINS)
    add_subdirectory(plugins)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

install(TARGETS openamp-dsp
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(DIRECTORY include/ DESTINATION include)