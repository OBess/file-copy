cmake_minimum_required(VERSION 3.22)
project(file-copy VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deploy)

set(Boost_NO_WARN_NEW_VERSIONS 1)
find_package(Boost COMPONENTS program_options REQUIRED)

add_executable(file-copy main.cpp)

if (MSVC)
    set_target_properties(file-copy PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
endif()

target_include_directories(file-copy PRIVATE include ${Boost_INCLUDE_DIR})
target_link_libraries(file-copy PRIVATE ${Boost_LIBRARIES} Boost::program_options)