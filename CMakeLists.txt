cmake_minimum_required(VERSION 3.21)
project(TikCanvas
    VERSION 1.0.0
    DESCRIPTION "High-performance MikroTik network operations canvas"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

if(MSVC)
    add_compile_options(/W4 /permissive- /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Concurrent Pdf)
qt_standard_project_setup()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(POPPLER_QT6 IMPORTED_TARGET poppler-qt6)
endif()

if(NOT POPPLER_QT6_FOUND)
    find_path(POPPLER_QT6_INCLUDE_DIR
        NAMES poppler-qt6.h
        PATH_SUFFIXES poppler/qt6 poppler-qt6
    )
    find_library(POPPLER_QT6_LIBRARY NAMES poppler-qt6)
    if(POPPLER_QT6_INCLUDE_DIR AND POPPLER_QT6_LIBRARY)
        set(POPPLER_QT6_FOUND TRUE)
        add_library(PkgConfig::POPPLER_QT6 INTERFACE IMPORTED)
        target_include_directories(PkgConfig::POPPLER_QT6 INTERFACE ${POPPLER_QT6_INCLUDE_DIR})
        target_link_libraries(PkgConfig::POPPLER_QT6 INTERFACE ${POPPLER_QT6_LIBRARY})
        message(STATUS "Poppler-Qt6 found manually: ${POPPLER_QT6_LIBRARY}")
    endif()
endif()

if(NOT POPPLER_QT6_FOUND)
    message(WARNING "Poppler-Qt6 not found. PDF rendering will be stubbed.")
    add_compile_definitions(TIKCANVAS_NO_POPPLER)
endif()

add_subdirectory(src)
