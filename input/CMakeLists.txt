# SPDX-FileCopyrightText: 2015 Ingo Ruhnke <grumbel@gmail.com>
# SPDX-License-Identifier: GPL-3.0-or-later

cmake_minimum_required(VERSION 3.15)

# Source of truth: top-level VERSION (e.g. "0.3.0-dev"), or
# -DPROJECT_VERSION_FULL=... from packaging (Nix may append .<revCount>+g<rev>).
if(NOT DEFINED PROJECT_VERSION_FULL)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
endif()

# project() needs a numeric version; strip -dev / +git / similar suffixes.
string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _project_version_cmake "${PROJECT_VERSION_FULL}")

project(wstinput VERSION ${_project_version_cmake} LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(WARNINGS "Enable extra compiler warnings" OFF)
option(WERROR "Treat warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings (inlined from former tinycmmc MaximumWarnings)
set(WSTINPUT_WARNINGS_CXX_FLAGS "")
if(WERROR)
  list(APPEND WSTINPUT_WARNINGS_CXX_FLAGS -Werror)
endif()
if(WARNINGS)
  list(APPEND WSTINPUT_WARNINGS_CXX_FLAGS -Wall -Wextra)
  if(CMAKE_COMPILER_IS_GNUCXX)
    list(APPEND WSTINPUT_WARNINGS_CXX_FLAGS
      -Wcast-align
      -Wcast-qual
      -Wconversion
      -Wctor-dtor-privacy
      -Wdisabled-optimization
      -Wdouble-promotion
      -Weffc++
      -Wformat=2
      -Winit-self
      -Winvalid-pch
      -Wlogical-op
      -Wmissing-format-attribute
      -Wmissing-noreturn
      -Wno-suggest-attribute=noreturn
      -Wno-unused-parameter
      -Wnon-virtual-dtor
      -Wold-style-cast
      -Woverloaded-virtual
      -Wpacked
      -Wredundant-decls
      -Wshadow
      -Wsign-promo
      -Wstrict-null-sentinel
      -Wsuggest-override
      -Wunreachable-code
      -Wzero-as-null-pointer-constant
      -pedantic
    )
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8)
      list(APPEND WSTINPUT_WARNINGS_CXX_FLAGS -Wint-in-bool-context)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND WSTINPUT_WARNINGS_CXX_FLAGS
      -Weverything
      -Wno-c++98-compat
      -Wno-c++98-compat-pedantic
      -Wno-float-equal
      -Wno-padded
      -Wno-weak-vtables
      -Wno-disabled-macro-expansion
      -Wno-documentation
      -Wno-reserved-id-macro
      -Wno-sign-conversion
      -Wno-gnu-zero-variadic-macro-arguments
      -Wno-poison-system-directories
      -Wno-reserved-identifier
      -Wno-documentation-unknown-command
      -Wno-inconsistent-missing-destructor-override
      -Wno-deprecated-dynamic-exception-spec
      -Wno-deprecated
      -Wno-switch-enum
      -Wno-covered-switch-default
      -Wno-exit-time-destructors
      -Wno-global-constructors
      -Wno-duplicate-enum
      -Wno-unused-parameter
      -Wno-old-style-cast
      -Wno-unreachable-code-break
      -Wno-double-promotion
      -Wno-unused-private-field
      -Wno-unused-exception-parameter
    )
  endif()
endif()

find_package(PkgConfig REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2 IMPORTED_TARGET)

find_library(CWIID_LIBRARY cwiid)

if(NOT TARGET logmich::logmich)
  find_package(logmich REQUIRED)
endif()
if(NOT TARGET prio::prio)
  find_package(prio REQUIRED)
endif()

file(GLOB WSTINPUT_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  include/wstinput/*.hpp)
file(GLOB WSTINPUT_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  src/*.cpp)

add_library(wstinput STATIC ${WSTINPUT_SOURCES})
set_target_properties(wstinput PROPERTIES PUBLIC_HEADER "${WSTINPUT_HEADERS}")
target_compile_options(wstinput PRIVATE ${WSTINPUT_WARNINGS_CXX_FLAGS})
target_include_directories(wstinput PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_include_directories(wstinput PRIVATE src/)
target_link_libraries(wstinput PUBLIC
  prio::prio
  logmich::logmich
  PkgConfig::SDL2)

if(CWIID_LIBRARY)
  target_compile_definitions(wstinput PUBLIC HAVE_CWIID)
  target_link_libraries(wstinput PUBLIC ${CWIID_LIBRARY})
endif()

if(BUILD_TESTS)
  find_package(GTest REQUIRED)
  include(GoogleTest)
  file(GLOB TEST_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*_test.cpp)
  add_executable(test_wstinput ${TEST_SOURCES})
  target_compile_options(test_wstinput PRIVATE ${WSTINPUT_WARNINGS_CXX_FLAGS})
  target_link_libraries(test_wstinput PRIVATE wstinput GTest::gtest GTest::gtest_main)
  gtest_discover_tests(test_wstinput)
endif()

add_library(wstinput::wstinput ALIAS wstinput)

# not when built as a subdirectory of a project that installs its own way
if(NOT DEFINED WST_INSTALL OR WST_INSTALL)
  # Export and install library (inlined from former tinycmmc)
  install(TARGETS wstinput
    EXPORT wstinput
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wstinput")

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/wstinput-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/wstinput-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstinput")
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/wstinput-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)
  export(EXPORT wstinput
    NAMESPACE wstinput::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/wstinput-targets.cmake")
  install(EXPORT wstinput
    FILE wstinput-targets.cmake
    NAMESPACE wstinput::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstinput")
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/wstinput-config-version.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/wstinput-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstinput")
endif()

# EOF #
