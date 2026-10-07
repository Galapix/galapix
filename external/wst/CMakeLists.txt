# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.15)

# Source of truth: top-level VERSION (e.g. "0.4.0-dev"), or
# -DPROJECT_VERSION_FULL=... from packaging (Nix may append .<revCount>+g<rev>).
# The modules pick up PROJECT_VERSION_FULL from here.
if(NOT DEFINED PROJECT_VERSION_FULL)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
endif()

string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _project_version_cmake "${PROJECT_VERSION_FULL}")

project(wst VERSION ${_project_version_cmake} LANGUAGES CXX C)

option(WST_DISPLAY "Build the wstdisplay module" ON)
option(WST_INPUT "Build the wstinput module" ON)
option(WST_GUI "Build the wstgui module (requires display and input)" ON)
option(WST_SPRITE "Build the wstsprite module (requires display)" ON)
option(WST_DEMOS "Build the demo applications (requires display)" ON)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  set(_wst_is_top_level ON)
else()
  set(_wst_is_top_level OFF)
endif()
option(WST_INSTALL "Generate install and export rules, off by default when wst is a subdirectory" ${_wst_is_top_level})

option(BUILD_TESTS "Build test cases" OFF)
option(BUILD_EXTRA "Build extra stuff" OFF)
option(WARNINGS "Enable extra compiler warnings" OFF)
option(WERROR "Treat warnings as errors" OFF)

if(WST_GUI AND NOT (WST_DISPLAY AND WST_INPUT))
  message(FATAL_ERROR "WST_GUI requires WST_DISPLAY and WST_INPUT")
endif()

if(WST_SPRITE AND NOT WST_DISPLAY)
  message(FATAL_ERROR "WST_SPRITE requires WST_DISPLAY")
endif()

if(BUILD_TESTS)
  enable_testing()
endif()

if(WST_DISPLAY)
  add_subdirectory(display)
endif()

if(WST_INPUT)
  add_subdirectory(input)
endif()

if(WST_GUI)
  add_subdirectory(gui)
endif()

if(WST_SPRITE)
  add_subdirectory(sprite)
endif()

if(WST_DEMOS AND WST_DISPLAY)
  add_subdirectory(demos)
endif()

# EOF #
