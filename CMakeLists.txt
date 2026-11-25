cmake_minimum_required(VERSION 3.16)

project(Lab3Graph LANGUAGES CXX)

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

include(CTest)
if(BUILD_TESTING)
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Extern/GoogleTest/CMakeLists.txt")
        message(FATAL_ERROR "GoogleTest is missing. Run: git submodule update --init --recursive")
    endif()
    set(BUILD_GMOCK OFF CACHE BOOL "Build Google Mock" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "Install GoogleTest" FORCE)
    if(MSVC)
        set(gtest_force_shared_crt ON CACHE BOOL "Use shared CRT" FORCE)
    endif()
    add_subdirectory(Extern/GoogleTest EXCLUDE_FROM_ALL)
    add_executable(graph_tests
        Tests/GraphTests.cpp
        Tests/SequenceQueueTests.cpp
        Tests/GraphAlgorithmsTests.cpp
        Tests/ShortestPathsTests.cpp
        Tests/TopologicalSortTests.cpp
        Tests/KruskalColoringTests.cpp
    )
    target_link_libraries(graph_tests PRIVATE Lab3::Graph GTest::gtest_main)
    if(MSVC)
        target_compile_options(graph_tests PRIVATE /W4 /permissive-)
    else()
        target_compile_options(graph_tests PRIVATE -Wall -Wextra -Wpedantic)
    endif()
    include(GoogleTest)
    gtest_discover_tests(graph_tests DISCOVERY_TIMEOUT 30)
endif()
