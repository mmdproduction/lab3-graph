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

add_subdirectory(Extern/GoogleTest EXCLUDE_FROM_ALL)
add_executable(graph_tests
    Tests/GraphTests.cpp
    Tests/SequenceQueueTests.cpp
    Tests/GraphAlgorithmsTests.cpp
    Tests/ShortestPathsTests.cpp
    Tests/TopologicalSortTests.cpp
    Tests/GraphTraceTests.cpp
    Tests/KruskalColoringTests.cpp
)
target_link_libraries(graph_tests PRIVATE Lab3::Graph GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(graph_tests DISCOVERY_TIMEOUT 30)



find_package(Qt6 6.2 QUIET COMPONENTS Widgets)
if(Qt6Widgets_FOUND)
        add_library(graph_gui_widgets
            App/Gui/GraphDocument.cpp
            App/Gui/TraceRunner.cpp
            App/Gui/MainWindow.cpp
            App/Gui/GraphCanvas.cpp
            App/Gui/Theme.cpp
        )
        target_link_libraries(graph_gui_widgets PUBLIC Lab3::Graph Qt6::Widgets)
        add_executable(graph_gui App/gui_main.cpp)
        target_link_libraries(graph_gui PRIVATE graph_gui_widgets)
        if(NOT MSVC)
            target_compile_options(graph_gui_widgets PRIVATE -Wall -Wextra -Wpedantic)
        endif()
        if(BUILD_TESTING)
            find_package(Qt6 REQUIRED COMPONENTS Test)
            add_executable(graph_gui_tests Tests/gui_test_main.cpp Tests/GuiModelTests.cpp Tests/GuiWindowTests.cpp)
            target_link_libraries(graph_gui_tests PRIVATE graph_gui_widgets Qt6::Test GTest::gtest)
            add_test(NAME graph_gui_tests COMMAND graph_gui_tests)
            set_tests_properties(graph_gui_tests PROPERTIES
                ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QPA_PLATFORMTHEME=generic"
                TIMEOUT 60)
        endif()
    else()
        message(STATUS "Qt6 Widgets not found; building the core only. Use -DBUILD_GUI=OFF to disable this check.")
    endif()

