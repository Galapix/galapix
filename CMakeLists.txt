# Copyright (C) 2025 Ingo Ruhnke <grumbel@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 3.14)

project(galapix CXX)

# --- Standard Install Directories ---
include(GNUInstallDirs)

# --- Configuration ---

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Suppress Boost warning on newer CMake versions (CMP0167)
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Build Options
option(BUILD_GALAPIX_SDL "Build galapix.sdl" ON)
option(BUILD_GALAPIX_GTK "Build galapix.gtk" ON)
option(BUILD_EXTRA_APPS "Build extra apps" ON)

# Testing
include(CTest)
option(BUILD_TESTS "Build tests" OFF)
if(BUILD_TESTS)
    set(BUILD_TESTING ON)
endif()

# Compiler Flags
if(CMAKE_BUILD_TYPE MATCHES Debug)
    add_compile_options(
        -O0 -g3
        -pedantic -Wall -Wextra
        -Wno-unused-parameter
        -Wcast-qual
    )
elseif(CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(-O3 -s)
endif()

include_directories(src)

# --- Dependencies ---

find_package(PkgConfig REQUIRED)

# Standard Libraries
find_package(Boost REQUIRED COMPONENTS filesystem system)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(JPEG REQUIRED)
# FindSQLite3 (CMake 3.14+) provides the SQLite::SQLite3 target
find_package(SQLite3 REQUIRED)

# PkgConfig Libraries
# We use IMPORTED_TARGET GLOBAL to create PkgConfig::<PREFIX> targets.
# This ensures that necessary defines (like MAGICKCORE_HDRI_ENABLE) are passed to the compiler.
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET GLOBAL sdl2)
pkg_check_modules(PNG REQUIRED IMPORTED_TARGET GLOBAL libpng)
pkg_check_modules(MAGICK REQUIRED IMPORTED_TARGET GLOBAL Magick++)
pkg_check_modules(CURL REQUIRED IMPORTED_TARGET GLOBAL libcurl)
pkg_check_modules(EXIF REQUIRED IMPORTED_TARGET GLOBAL libexif)

# Optional: Space Navigator
find_path(SPNAV_INCLUDE_DIR spnav.h)
find_library(SPNAV_LIBRARY spnav)
if(SPNAV_INCLUDE_DIR AND SPNAV_LIBRARY)
    message(STATUS "Space Navigator (libspnav) found.")
    set(HAVE_SPACE_NAVIGATOR TRUE)
endif()

# --- Sources ---

file(GLOB UTIL_SOURCES
    src/util/*.cpp
    src/plugins/*.cpp
    src/lisp/*.cpp
    src/math/*.cpp
)

file(GLOB CORE_SOURCES
    src/database/*.cpp
    src/display/*.cpp
    src/galapix/*.cpp
    src/job/*.cpp
    src/jobs/*.cpp
    src/sqlite/*.cpp
    src/tools/*.cpp
)
# Exclude entry points from the library to avoid duplicates
list(FILTER CORE_SOURCES EXCLUDE REGEX "src/galapix/galapix.cpp$")
list(FILTER CORE_SOURCES EXCLUDE REGEX "src/galapix/viewer.cpp$")

if(HAVE_SPACE_NAVIGATOR)
    list(APPEND CORE_SOURCES src/spnav/space_navigator.cpp)
endif()

set(GALAPIX_BASE_SOURCES
    src/display/framebuffer.cpp
    src/galapix/galapix.cpp
    src/galapix/viewer.cpp
)

set(SDL_SPECIFIC_SOURCES src/sdl/sdl_window.cpp src/sdl/sdl_viewer.cpp)
set(GTK_SPECIFIC_SOURCES src/gtk/gtk_viewer.cpp src/gtk/gtk_viewer_widget.cpp)

# --- Library Targets ---

# 1. galapix_util
add_library(galapix_util STATIC ${UTIL_SOURCES})
# Linking against PkgConfig targets pulls in Includes and Defines (fixing the ImageMagick error)
target_link_libraries(galapix_util PUBLIC
    PkgConfig::PNG
    PkgConfig::SDL2
    PkgConfig::MAGICK
    PkgConfig::CURL
)

# 2. galapix_core
add_library(galapix_core STATIC ${CORE_SOURCES})
target_link_libraries(galapix_core PUBLIC
    galapix_util
    OpenGL::GL
    GLEW::GLEW
    JPEG::JPEG
    SQLite::SQLite3
    PkgConfig::EXIF
    Boost::filesystem
    Boost::system
)

if(HAVE_SPACE_NAVIGATOR)
    target_compile_definitions(galapix_core PUBLIC HAVE_SPACE_NAVIGATOR=1)
    target_include_directories(galapix_core PUBLIC ${SPNAV_INCLUDE_DIR})
    target_link_libraries(galapix_core PUBLIC ${SPNAV_LIBRARY})
endif()

# --- Executables ---

# 1. Galapix SDL
if(BUILD_GALAPIX_SDL)
    add_executable(galapix.sdl ${SDL_SPECIFIC_SOURCES} ${GALAPIX_BASE_SOURCES})
    target_compile_definitions(galapix.sdl PRIVATE GALAPIX_SDL)
    target_link_libraries(galapix.sdl PRIVATE galapix_core)
    install(TARGETS galapix.sdl RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

# 2. Galapix GTK
if(BUILD_GALAPIX_GTK)
    pkg_check_modules(GTKMM IMPORTED_TARGET GLOBAL gtkmm-2.4 libglademm-2.4 gtkglextmm-1.2)

    if(GTKMM_FOUND)
        add_executable(galapix.gtk ${GTK_SPECIFIC_SOURCES} ${GALAPIX_BASE_SOURCES})
        target_compile_definitions(galapix.gtk PRIVATE GALAPIX_GTK)
        target_link_libraries(galapix.gtk PRIVATE galapix_core PkgConfig::GTKMM)
        install(TARGETS galapix.gtk RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    else()
        message(WARNING "GTK/GTKMM dependencies not found. Skipping galapix.gtk")
    endif()
endif()

# 3. Tests
if(BUILD_TESTING)
    file(GLOB TEST_SOURCES "test/*_test.cpp")
    foreach(TEST_SRC ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SRC})
        target_link_libraries(${TEST_NAME} PRIVATE galapix_core galapix_util)
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
endif()

# 4. Extra Apps
if(BUILD_EXTRA_APPS)
  file(GLOB EXTRA_SOURCES "extra/*.cpp")
  foreach(APP_SRC ${EXTRA_SOURCES})
    get_filename_component(APP_NAME ${APP_SRC} NAME_WE)
    add_executable(${APP_NAME} ${APP_SRC})
    target_link_libraries(${APP_NAME} PRIVATE galapix_core galapix_util)
    install(TARGETS ${APP_NAME} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endforeach()

  if(EXISTS "${CMAKE_SOURCE_DIR}/extra/imagescaler/imagescaler.cpp")
    add_executable(imagescaler "extra/imagescaler/imagescaler.cpp")
    target_link_libraries(imagescaler PRIVATE galapix_core galapix_util)
    install(TARGETS imagescaler RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
  endif()
endif()

# EOF #
