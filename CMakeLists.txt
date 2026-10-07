# surf - Software surface library
# Copyright (C) 2008-2020 Ingo Ruhnke <grumbel@gmail.com>
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

cmake_minimum_required(VERSION 3.21)

# Source of truth: top-level VERSION file (e.g. "0.2.0-dev"), or
# -DPROJECT_VERSION_FULL=... from the packaging (Nix flake appends
# .<revCount>+g<rev>).
# A parent project's PROJECT_VERSION_FULL isn't ours when we are a subdirectory
if(NOT DEFINED PROJECT_VERSION_FULL OR NOT CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
endif()

# CMake project() VERSION expects major[.minor[.patch[.tweak]]]; strip
# any -dev / +git suffix for the numeric project version.
string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _project_version_cmake "${PROJECT_VERSION_FULL}")

project(surf VERSION ${_project_version_cmake} LANGUAGES CXX)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(WITH_MAGICKXX "Enable ImageMagick" OFF)
option(WITH_EXIF "Enable EXIF support" ON)
option(WITH_EXEC "Enable plugins needing exec() support" OFF)
option(WITH_STB "Use the header-only stb_image and stb_image_write for PNG and JPEG instead of libpng and libjpeg" OFF)
option(SURF_INSTALL "Generate install and export rules, off by default when surf is a subdirectory" ${PROJECT_IS_TOP_LEVEL})
option(BUILD_TESTS "Build test cases" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(BUILD_EXTRA "Build extra stuff" OFF)
option(WARNINGS "Enable extra compiler warnings" OFF)
option(WERROR "Treat warnings as errors" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings (inlined from former tinycmmc)
set(SURF_WARNINGS_CXX_FLAGS "")
if(WERROR)
  list(APPEND SURF_WARNINGS_CXX_FLAGS -Werror)
endif()
if(WARNINGS)
  list(APPEND SURF_WARNINGS_CXX_FLAGS -Wall -Wextra)
  if(CMAKE_COMPILER_IS_GNUCXX)
    list(APPEND SURF_WARNINGS_CXX_FLAGS
      -Wcast-align -Wcast-qual -Wconversion -Wctor-dtor-privacy
      -Wdisabled-optimization -Wdouble-promotion -Weffc++ -Wformat=2
      -Winit-self -Winvalid-pch -Wlogical-op -Wmissing-format-attribute
      -Wmissing-noreturn -Wno-suggest-attribute=noreturn -Wno-unused-parameter
      -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual -Wpacked
      -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel
      -Wsuggest-override -Wunreachable-code -Wzero-as-null-pointer-constant
      -pedantic)
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8)
      list(APPEND SURF_WARNINGS_CXX_FLAGS -Wint-in-bool-context)
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND SURF_WARNINGS_CXX_FLAGS
      -Weverything -Wno-c++98-compat -Wno-c++98-compat-pedantic
      -Wno-float-equal -Wno-padded -Wno-weak-vtables
      -Wno-disabled-macro-expansion -Wno-documentation -Wno-reserved-id-macro
      -Wno-sign-conversion -Wno-gnu-zero-variadic-macro-arguments
      -Wno-poison-system-directories -Wno-reserved-identifier
      -Wno-documentation-unknown-command
      -Wno-inconsistent-missing-destructor-override
      -Wno-deprecated-dynamic-exception-spec -Wno-deprecated
      -Wno-switch-enum -Wno-covered-switch-default
      -Wno-exit-time-destructors -Wno-global-constructors
      -Wno-duplicate-enum -Wno-unused-parameter -Wno-old-style-cast
      -Wno-unreachable-code-break -Wno-double-promotion
      -Wno-unused-private-field -Wno-unused-exception-parameter)
  endif()
endif()

find_package(PkgConfig REQUIRED)

if(WITH_STB)
  # nixpkgs installs the headers to include/stb/
  find_path(STB_INCLUDE_DIR stb_image.h PATH_SUFFIXES stb REQUIRED)
  if(NOT EXISTS "${STB_INCLUDE_DIR}/stb_image_write.h")
    message(FATAL_ERROR "stb_image_write.h not found next to ${STB_INCLUDE_DIR}/stb_image.h")
  endif()
  if(WITH_EXEC)
    message(FATAL_ERROR "WITH_EXEC needs libpng and can't be combined with WITH_STB")
  endif()
else()
  find_package(JPEG REQUIRED)
  find_package(PNG REQUIRED)
endif()

if(WITH_EXIF)
  pkg_search_module(EXIF libexif IMPORTED_TARGET)
endif()

if(WITH_MAGICKXX)
  pkg_search_module(MAGICKXX Magick++ GraphicsMagick++ IMPORTED_TARGET)
endif()

# The targets already exist when surf is a subdirectory next to them
if(NOT TARGET geom::geom)
  find_package(geom REQUIRED)
endif()
if(NOT TARGET logmich::logmich)
  find_package(logmich REQUIRED)
endif()

set(SURF_DEFINES)

set(SURF_SOURCES
  src/blend.cpp
  src/blend_scaled.cpp
  src/blendfunc.cpp
  src/blit.cpp
  src/channel.cpp
  src/color.cpp
  src/convert.cpp
  src/fill.cpp
  src/palette.cpp
  src/pixel_format.cpp
  src/plugins/dds.cpp
  src/plugins/dds_surface.cpp
  src/plugins/exif.cpp
  src/plugins/imagemagick.cpp
  src/plugins/pnm.cpp
  src/save.cpp
  src/software_surface.cpp
  src/software_surface_factory.cpp
  src/transform.cpp
  src/util/filesystem.cpp
  )

set(SURF_LIBPNG_LIBJPEG_SOURCES
  src/plugins/file_jpeg_compressor.cpp
  src/plugins/file_jpeg_decompressor.cpp
  src/plugins/jpeg.cpp
  src/plugins/jpeg_compressor.cpp
  src/plugins/jpeg_decompressor.cpp
  src/plugins/jpeg_memory_dest.cpp
  src/plugins/jpeg_memory_src.cpp
  src/plugins/mem_jpeg_compressor.cpp
  src/plugins/mem_jpeg_decompressor.cpp
  src/plugins/png.cpp
  )

if(WITH_STB)
  list(APPEND SURF_SOURCES src/plugins/stb.cpp)
  list(APPEND SURF_DEFINES "HAVE_STB")
else()
  list(APPEND SURF_SOURCES ${SURF_LIBPNG_LIBJPEG_SOURCES})
endif()

set(SURF_EXEC_SOURCES
  src/plugins/kra.cpp
  src/plugins/rsvg.cpp
  src/plugins/ufraw.cpp
  src/plugins/vidthumb.cpp
  src/plugins/xcf.cpp
  src/util/exec.cpp
  )

if(WITH_EXEC)
  list(APPEND SURF_SOURCES ${SURF_EXEC_SOURCES})
  list(APPEND SURF_DEFINES "HAVE_EXEC")
endif()

add_library(surf STATIC ${SURF_SOURCES})
add_library(surf::surf ALIAS surf)
target_compile_features(surf PUBLIC cxx_std_20)
target_compile_definitions(surf PRIVATE ${SURF_DEFINES})
target_compile_options(surf PRIVATE ${SURF_WARNINGS_CXX_FLAGS})
target_link_libraries(surf PUBLIC
  geom::geom
  logmich::logmich
  )
if(WITH_STB)
  target_include_directories(surf SYSTEM PRIVATE ${STB_INCLUDE_DIR})
else()
  target_link_libraries(surf PUBLIC PNG::PNG JPEG::JPEG)
endif()
target_include_directories(surf PUBLIC
  $<INSTALL_INTERFACE:include>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  )

# Internal headers and un-prefixed includes, users of the library
# have to use <surf/...>, same as with the installed headers
set(SURF_PRIVATE_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/include/surf
  )
target_include_directories(surf PRIVATE ${SURF_PRIVATE_INCLUDE_DIRS})

if(MAGICKXX_FOUND)
  target_link_libraries(surf PUBLIC PkgConfig::MAGICKXX)
  target_compile_definitions(surf PUBLIC "HAVE_MAGICKXX")
endif()

if(EXIF_FOUND)
  target_link_libraries(surf PUBLIC PkgConfig::EXIF)
  target_compile_definitions(surf PUBLIC "HAVE_EXIF")
endif()

if(BUILD_TESTS)
  find_package(GTest REQUIRED)
  pkg_search_module(SDL2 REQUIRED sdl2 IMPORTED_TARGET)

  file(GLOB TEST_SURF_SOURCES test/*_test.cpp)
  # tests of the image backend that isn't built
  if(WITH_STB)
    list(FILTER TEST_SURF_SOURCES EXCLUDE REGEX "/(png|jpeg)_test\\.cpp$")
  else()
    list(FILTER TEST_SURF_SOURCES EXCLUDE REGEX "/stb_test\\.cpp$")
  endif()
  add_executable(test_surf ${TEST_SURF_SOURCES})
  target_include_directories(test_surf PRIVATE ${SURF_PRIVATE_INCLUDE_DIRS})
  target_compile_options(test_surf PRIVATE ${SURF_WARNINGS_CXX_FLAGS})
  target_compile_definitions(test_surf PRIVATE ${SURF_DEFINES})
  target_link_libraries(test_surf
    surf
    PkgConfig::SDL2
    GTest::GTest
    GTest::Main)

  enable_testing()
  add_test(NAME test_surf
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    COMMAND test_surf)
endif()

if(BUILD_BENCHMARKS)
  find_package(benchmark REQUIRED)

  file(GLOB BENCHMARK_SURF_SOURCES benchmark/*.cpp)
  add_executable(benchmark_surf ${BENCHMARK_SURF_SOURCES})
  target_include_directories(benchmark_surf PRIVATE ${SURF_PRIVATE_INCLUDE_DIRS})
  target_compile_options(benchmark_surf PRIVATE ${SURF_WARNINGS_CXX_FLAGS})
  target_link_libraries(benchmark_surf
    surf
    benchmark::benchmark)
endif()

if(BUILD_EXTRA)
  file(GLOB EXTRA_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    extra/*.cpp)
  foreach(SRC ${EXTRA_SOURCES})
    get_filename_component(SRC_BASENAME ${SRC} NAME_WE)
    add_executable(${SRC_BASENAME} ${SRC})
    target_include_directories(${SRC_BASENAME} PRIVATE ${SURF_PRIVATE_INCLUDE_DIRS})
    target_compile_options(${SRC_BASENAME} PRIVATE ${SURF_WARNINGS_CXX_FLAGS})
    target_link_libraries(${SRC_BASENAME} surf)
    if(SURF_INSTALL)
      install(TARGETS ${SRC_BASENAME}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
  endforeach(SRC)
endif()

if(SURF_INSTALL)
  install(DIRECTORY include/. DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

  # Export and install library (inlined from former tinycmmc)
  install(TARGETS surf
    EXPORT surf
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/surf")

  # Dependencies of the exported target, in surf-config.cmake
  set(SURF_CONFIG_DEPENDENCIES
    "find_dependency(geom)"
    "find_dependency(logmich)")
  if(NOT WITH_STB)
    list(APPEND SURF_CONFIG_DEPENDENCIES
      "find_dependency(JPEG)"
      "find_dependency(PNG)")
  endif()
  if(MAGICKXX_FOUND OR EXIF_FOUND)
    list(APPEND SURF_CONFIG_DEPENDENCIES "find_dependency(PkgConfig)")
  endif()
  if(MAGICKXX_FOUND)
    list(APPEND SURF_CONFIG_DEPENDENCIES "pkg_search_module(MAGICKXX REQUIRED Magick++ GraphicsMagick++ IMPORTED_TARGET)")
  endif()
  if(EXIF_FOUND)
    list(APPEND SURF_CONFIG_DEPENDENCIES "pkg_search_module(EXIF REQUIRED libexif IMPORTED_TARGET)")
  endif()
  list(JOIN SURF_CONFIG_DEPENDENCIES "\n" SURF_CONFIG_DEPENDENCIES)

  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/surf-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/surf-config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/surf")
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/surf-config-version.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMinorVersion)
  export(EXPORT surf
    NAMESPACE surf::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/surf-targets.cmake")
  install(EXPORT surf
    FILE surf-targets.cmake
    NAMESPACE surf::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/surf")
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/surf-config-version.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/surf-config.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/surf")
endif()

# EOF #
