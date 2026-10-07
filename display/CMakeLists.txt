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

project(wstdisplay VERSION ${_project_version_cmake} LANGUAGES CXX C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(BUILD_TESTS "Build test cases" OFF)
option(BUILD_EXTRA "Build extra stuff" OFF)
option(WARNINGS "Enable extra compiler warnings" OFF)
option(WERROR "Treat warnings as errors" OFF)

if(BUILD_TESTS)
  # add 'make test' target, use 'make test ARGS="-V"' or 'ctest -V' for verbose
  enable_testing()
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings (inlined from former tinycmmc MaximumWarnings)
set(WSTDISPLAY_WARNINGS_CXX_FLAGS "")
if(WERROR)
  list(APPEND WSTDISPLAY_WARNINGS_CXX_FLAGS -Werror)
endif()
if(WARNINGS)
  list(APPEND WSTDISPLAY_WARNINGS_CXX_FLAGS -Wall -Wextra)
  if(CMAKE_COMPILER_IS_GNUCXX)
    list(APPEND WSTDISPLAY_WARNINGS_CXX_FLAGS
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
      list(APPEND WSTDISPLAY_WARNINGS_CXX_FLAGS -Wint-in-bool-context)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND WSTDISPLAY_WARNINGS_CXX_FLAGS
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

find_package(Threads REQUIRED)
find_package(Freetype REQUIRED)
# GL is loaded at runtime through the loader given to Device, the GL
# library is only linked where one exists, Emscripten and Android
# provide the functions through SDL and EGL
find_package(OpenGL)
find_package(PkgConfig REQUIRED)

pkg_search_module(SDL2 REQUIRED sdl2 IMPORTED_TARGET)

if(NOT TARGET babyxml::babyxml)
  find_package(babyxml REQUIRED)
endif()
if(NOT TARGET geom::geom)
  find_package(geom REQUIRED)
endif()
if(NOT TARGET surf::surf)
  find_package(surf REQUIRED)
endif()
if(NOT TARGET logmich::logmich)
  find_package(logmich REQUIRED)
endif()

file(GLOB WSTDISPLAY_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  src/*.cpp
  src/font/*.cpp
  external/glad/src/gl.c
  )
add_library(wstdisplay STATIC ${WSTDISPLAY_SOURCES})
target_include_directories(wstdisplay PRIVATE
  src/
  external/glad/include
  )
target_include_directories(wstdisplay PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  )
target_compile_options(wstdisplay PRIVATE $<$<COMPILE_LANGUAGE:CXX>:${WSTDISPLAY_WARNINGS_CXX_FLAGS}>)
# glad is C; silence some warnings
set_source_files_properties(external/glad/src/gl.c PROPERTIES
  COMPILE_FLAGS "-Wno-pedantic -Wno-unused-function")
target_link_libraries(wstdisplay PUBLIC
  babyxml::babyxml
  geom::geom
  surf::surf
  logmich::logmich
  PkgConfig::SDL2
  Threads::Threads)
if(TARGET OpenGL::GL)
  target_link_libraries(wstdisplay PUBLIC OpenGL::GL)
endif()
# FreeType is not part of the public headers
target_link_libraries(wstdisplay PRIVATE
  Freetype::Freetype)

if(BUILD_TESTS)
  find_package(GTest REQUIRED)
  include(GoogleTest)
  file(GLOB TEST_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    test/*_test.cpp)
  add_executable(test_wstdisplay ${TEST_SOURCES})
  target_compile_options(test_wstdisplay PRIVATE ${WSTDISPLAY_WARNINGS_CXX_FLAGS})
  target_include_directories(test_wstdisplay PRIVATE src/ include/wstdisplay include/wstsystem)
  target_link_libraries(test_wstdisplay PRIVATE wstdisplay GTest::gtest GTest::gtest_main)
  gtest_discover_tests(test_wstdisplay)
endif()

if(BUILD_EXTRA)
  file(GLOB EXTRA_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    extra/*.cpp)
  foreach(SRC ${EXTRA_SOURCES})
    get_filename_component(SRC_BASENAME ${SRC} NAME_WE)
    add_executable(${SRC_BASENAME} ${SRC})
    set_target_properties(${SRC_BASENAME} PROPERTIES OUTPUT_NAME "${PROJECT_NAME}-${SRC_BASENAME}")
    target_link_libraries(${SRC_BASENAME} PRIVATE wstdisplay)
    install(TARGETS ${SRC_BASENAME}
      RUNTIME DESTINATION ${CMAKE_INSTALL_FULL_BINDIR})
  endforeach(SRC)
endif()

add_library(wstdisplay::wstdisplay ALIAS wstdisplay)

# not when built as a subdirectory of a project that installs its own way
if(NOT DEFINED WST_INSTALL OR WST_INSTALL)
  # PUBLIC_HEADER can't handle subdirectories, so do this manually
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/"
    FILES_MATCHING
    PATTERN "*.hpp"
    )

  # Export and install library (inlined from former tinycmmc)
  install(TARGETS wstdisplay
    EXPORT wstdisplay
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/wstdisplay")

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/wstdisplay-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/wstdisplay-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstdisplay")
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/wstdisplay-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)
  export(EXPORT wstdisplay
    NAMESPACE wstdisplay::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/wstdisplay-targets.cmake")
  install(EXPORT wstdisplay
    FILE wstdisplay-targets.cmake
    NAMESPACE wstdisplay::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstdisplay")
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/wstdisplay-config-version.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/wstdisplay-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/wstdisplay")
endif()

# EOF #
