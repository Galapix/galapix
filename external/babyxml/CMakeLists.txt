# babyxml - tiny parser for XML-like dialect
# Copyright (C) 2009-2020 Ingo Ruhnke <grumbel@gmail.com>
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

cmake_minimum_required(VERSION 3.6)

# Source of truth: top-level VERSION (e.g. "0.0.0-dev"), or
# -DPROJECT_VERSION_FULL=... from packaging (Nix may append .<revCount>+g<rev>).
if(NOT DEFINED PROJECT_VERSION_FULL)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
endif()

# project() needs a numeric version; strip -dev / +git / similar suffixes.
string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _project_version_cmake "${PROJECT_VERSION_FULL}")

project(babyxml VERSION ${_project_version_cmake} LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(BUILD_EXTRA "Build extra stuff" OFF)
option(WARNINGS "Enable extra compiler warnings" OFF)
option(WERROR "Treat warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BABYXML_WARNINGS_CXX_FLAGS "")
if(WERROR)
  list(APPEND BABYXML_WARNINGS_CXX_FLAGS -Werror)
endif()
if(WARNINGS)
  list(APPEND BABYXML_WARNINGS_CXX_FLAGS -Wall -Wextra -pedantic)
endif()

add_library(babyxml STATIC
  src/babyxml.cpp
  src/reader.cpp)
target_include_directories(babyxml PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_options(babyxml PRIVATE ${BABYXML_WARNINGS_CXX_FLAGS})
set_target_properties(babyxml PROPERTIES
  PUBLIC_HEADER "include/babyxml.hpp")

if(BUILD_EXTRA)
  add_executable(babyxml_exe extra/babyxml.cpp)
  set_target_properties(babyxml_exe PROPERTIES OUTPUT_NAME "babyxml")
  target_link_libraries(babyxml_exe PRIVATE babyxml)
  target_compile_options(babyxml_exe PRIVATE ${BABYXML_WARNINGS_CXX_FLAGS})
  install(TARGETS babyxml_exe
    RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
endif()

# Export and install library (inlined from former tinycmmc)
install(TARGETS babyxml
  EXPORT babyxml
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

add_library(babyxml::babyxml ALIAS babyxml)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/babyxml-config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/babyxml-config.cmake"
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/babyxml")
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/babyxml-config-version.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMinorVersion)
export(EXPORT babyxml
  NAMESPACE babyxml::
  FILE "${CMAKE_CURRENT_BINARY_DIR}/babyxml-targets.cmake")
install(EXPORT babyxml
  FILE babyxml-targets.cmake
  NAMESPACE babyxml::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/babyxml")
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/babyxml-config-version.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/babyxml-config.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/babyxml")

# EOF #
