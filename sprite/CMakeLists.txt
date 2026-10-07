# SPDX-FileCopyrightText: 2026 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: LGPL-3.0-or-later

cmake_minimum_required(VERSION 3.15)

if(NOT DEFINED PROJECT_VERSION_FULL)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/../VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
endif()
string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _project_version_cmake "${PROJECT_VERSION_FULL}")

project(wstsprite VERSION ${_project_version_cmake} LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(WSTSPRITE_WARNINGS_CXX_FLAGS "")
if(WERROR)
  list(APPEND WSTSPRITE_WARNINGS_CXX_FLAGS -Werror)
endif()
if(WARNINGS)
  list(APPEND WSTSPRITE_WARNINGS_CXX_FLAGS -Wall -Wextra -Wconversion -Wshadow -Wnon-virtual-dtor
    -Wold-style-cast -Woverloaded-virtual -Wsuggest-override -pedantic)
endif()

if(NOT TARGET wstdisplay::wstdisplay)
  find_package(wstdisplay REQUIRED)
endif()
if(NOT TARGET prio::prio)
  find_package(prio REQUIRED)
endif()
if(NOT TARGET logmich::logmich)
  find_package(logmich REQUIRED)
endif()

file(GLOB WSTSPRITE_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} src/*.cpp)
add_library(wstsprite STATIC ${WSTSPRITE_SOURCES})
target_include_directories(wstsprite PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(wstsprite PRIVATE src/)
target_compile_options(wstsprite PRIVATE ${WSTSPRITE_WARNINGS_CXX_FLAGS})
target_link_libraries(wstsprite PUBLIC
  wstdisplay::wstdisplay
  prio::prio
  logmich::logmich)

if(BUILD_TESTS)
  find_package(GTest REQUIRED)
  include(GoogleTest)
  file(GLOB TEST_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*_test.cpp)
  add_executable(test_wstsprite ${TEST_SOURCES})
  target_compile_options(test_wstsprite PRIVATE ${WSTSPRITE_WARNINGS_CXX_FLAGS})
  target_link_libraries(test_wstsprite PRIVATE wstsprite GTest::gtest GTest::gtest_main)
  gtest_discover_tests(test_wstsprite)
endif()

add_library(wstsprite::wstsprite ALIAS wstsprite)

# not when built as a subdirectory of a project that installs its own way
if(NOT DEFINED WST_INSTALL OR WST_INSTALL)
  install(TARGETS wstsprite
    EXPORT wstsprite
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/"
    FILES_MATCHING PATTERN "*.hpp")

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/wstsprite-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/wstsprite-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstsprite")
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/wstsprite-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)
  export(EXPORT wstsprite
    NAMESPACE wstsprite::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/wstsprite-targets.cmake")
  install(EXPORT wstsprite
    FILE wstsprite-targets.cmake
    NAMESPACE wstsprite::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstsprite")
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/wstsprite-config-version.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/wstsprite-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstsprite")
endif()

# EOF #
