cmake_minimum_required(VERSION 3.16)

project(Lab3::Graph LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(graph INTERFACE)
add_library(Lab3::Graph ALIAS graph)
target_compile_features(graph INTERFACE cxx_std_20)
target_include_directories(graph INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/Extern/AVL/AVL"
)





# add_subdirectory(Extern/GoogleTest EXCLUDE_FROM_ALL)
# add_executable(graph_tests ${GRAPH_TEST_SOURCES})
# target_link_libraries(graph_tests PRIVATE Lab3::Graph GTest::gtest_main)
# include(GoogleTest)
