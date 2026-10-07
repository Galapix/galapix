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

project(wstgui VERSION ${_project_version_cmake} LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(BUILD_EXTRA "Build extra stuff" OFF)
option(WARNINGS "Enable extra compiler warnings" OFF)
option(WERROR "Treat warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings (inlined from former tinycmmc MaximumWarnings)
set(WARNINGS_CXX_FLAGS "")
if(WERROR)
  list(APPEND WARNINGS_CXX_FLAGS -Werror)
endif()
if(WARNINGS)
  list(APPEND WARNINGS_CXX_FLAGS -Wall -Wextra)
  if(CMAKE_COMPILER_IS_GNUCXX)
    list(APPEND WARNINGS_CXX_FLAGS
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
      list(APPEND WARNINGS_CXX_FLAGS -Wint-in-bool-context)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND WARNINGS_CXX_FLAGS
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

if(NOT TARGET sexp::sexp)
  find_package(sexp REQUIRED)
endif()
if(NOT TARGET babyxml::babyxml)
  find_package(babyxml REQUIRED)
endif()
if(NOT TARGET logmich::logmich)
  find_package(logmich REQUIRED)
endif()
if(NOT TARGET geom::geom)
  find_package(geom REQUIRED)
endif()
if(NOT TARGET prio::prio)
  find_package(prio REQUIRED)
endif()
if(NOT TARGET surf::surf)
  find_package(surf REQUIRED)
endif()
if(NOT TARGET wstdisplay::wstdisplay)
  find_package(wstdisplay REQUIRED)
endif()
if(NOT TARGET wstinput::wstinput)
  find_package(wstinput REQUIRED)
endif()

file(GLOB WSTGUI_HEADERS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  include/wstgui/*.hpp)
file(GLOB WSTGUI_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  src/*.cpp)

add_library(wstgui STATIC ${WSTGUI_SOURCES})
set_target_properties(wstgui PROPERTIES PUBLIC_HEADER "${WSTGUI_HEADERS}")
target_compile_options(wstgui PRIVATE ${WARNINGS_CXX_FLAGS})
target_include_directories(wstgui PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )
target_include_directories(wstgui PRIVATE src/)
target_link_libraries(wstgui PUBLIC
  wstinput::wstinput
  geom::geom
  prio::prio
  PkgConfig::SDL2
  wstdisplay::wstdisplay
  )

if(BUILD_EXTRA)
  file(GLOB EXTRA_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    extra/*.cpp)
  foreach(SRC ${EXTRA_SOURCES})
    get_filename_component(SRC_BASENAME ${SRC} NAME_WE)
    add_executable("${SRC_BASENAME}_exe" ${SRC})
    set_target_properties("${SRC_BASENAME}_exe" PROPERTIES
      OUTPUT_NAME "${SRC_BASENAME}"
      CXX_STANDARD 20
      CXX_STANDARD_REQUIRED ON
      CXX_EXTENSIONS OFF)
    target_link_libraries("${SRC_BASENAME}_exe" PRIVATE wstgui)
    install(TARGETS "${SRC_BASENAME}_exe"
      RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
  endforeach(SRC)
endif()

if(BUILD_TESTS)
  find_package(GTest REQUIRED)
  include(GoogleTest)
  file(GLOB TEST_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*_test.cpp)
  add_executable(test_wstgui ${TEST_SOURCES})
  target_compile_options(test_wstgui PRIVATE ${WARNINGS_CXX_FLAGS})
  target_link_libraries(test_wstgui PRIVATE wstgui GTest::gtest GTest::gtest_main)
  gtest_discover_tests(test_wstgui)
endif()

add_library(wstgui::wstgui ALIAS wstgui)

# not when built as a subdirectory of a project that installs its own way
if(NOT DEFINED WST_INSTALL OR WST_INSTALL)
  # Export and install library (inlined from former tinycmmc)
  install(TARGETS wstgui
    EXPORT wstgui
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wstgui")

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/wstgui-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/wstgui-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstgui")
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/wstgui-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)
  export(EXPORT wstgui
    NAMESPACE wstgui::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/wstgui-targets.cmake")
  install(EXPORT wstgui
    FILE wstgui-targets.cmake
    NAMESPACE wstgui::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstgui")
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/wstgui-config-version.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/wstgui-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstgui")
endif()

# EOF #
