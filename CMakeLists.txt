# Galapix - an image viewer for large image collections
# Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
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

cmake_minimum_required(VERSION 3.0)
project(galapix VERSION 0.3.0)

if(POLICY CMP0072)
  # FindOpenGL prefers GLVND by default when available
  cmake_policy(SET CMP0072 NEW)
endif()

include(GNUInstallDirs)

option(BUILD_GALAPIX_SDL "Build galapix.sdl" ON)
option(BUILD_GALAPIX_GTK "Build galapix.gtk" OFF)
option(BUILD_TESTS "Build tests" OFF)
option(BUILD_EXTRA "Build extra apps" OFF)
option(USE_OPENGLES2 "Use OpenGLES2 instead of OpenGL" OFF)
option(WARNINGS "Switch on extra warnings" OFF)
option(WERROR "Treat all warnings as errors" OFF)
option(THREADSANITIZER "Thread Sanitizer" OFF)

if(WARNINGS)
  list(APPEND -Wall -Wextra)
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
      -Wmissing-include-dirs
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
      -Wuseless-cast
      -Wzero-as-null-pointer-constant
      -pedantic
      )
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 8)
      list(APPEND WARNINGS_CXX_FLAGS -Wint-in-bool-context)
    endif()
    if(Boost_VERSION LESS 106500)
      list(APPEND WARNINGS_CXX_FLAGS
        -Wno-implicit-fallthrough
        )
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    list(APPEND WARNINGS_CXX_FLAGS
      -Weverything

      # flags that we deliberately ignore
     -Wno-c++98-compat
     -Wno-c++98-compat-pedantic
     -Wno-float-equal
     -Wno-padded
     -Wno-weak-vtables
     -Wno-disabled-macro-expansion
     -Wno-documentation
     -Wno-reserved-id-macro
     -Wno-sign-conversion

      # warnings that should probably be fixed in code
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

if(THREADSANITIZER)
  add_compile_options(-fsanitize=thread)
endif()

if(WERROR)
  set(WARNINGS_CXX_FLAGS ${WARNINGS_CXX_FLAGS}
    -Werror)
endif()

if(CMAKE_VERSION VERSION_GREATER 3.6)
  if (USE_CLANG_TIDY)
    find_program(
      CLANG_TIDY_EXE
      NAMES "clang-tidy"
      DOC "Path to clang-tidy executable"
      )

    if(NOT CLANG_TIDY_EXE)
      message(FATAL_ERROR "clang-tidy not found.")
    else()
      message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
      set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    endif()
  endif()
endif()

add_compile_options(-std=c++17)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CheckIncludeFile)

find_package(Threads REQUIRED)
find_package(Boost COMPONENTS system filesystem signals REQUIRED)
find_package(CURL REQUIRED)
find_package(JPEG REQUIRED)
find_package(PNG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_search_module(SQLITE3 REQUIRED sqlite3)

# older versions don't have a mhash.pc
pkg_search_module(MHASH mhash)
if(NOT MHASH_FOUND)
  find_library(MHASH_LIBRARIES mhash)
  message(STATUS "Found mhash: ${MHASH_LIBRARIES}")
  if(MHASH_LIBRARIES-NOTFOUND)
    message(FATAL_ERROR "required library mhash not found")
  endif()
endif()

# Not using find_package(), see:
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=776832
# find_package(ImageMagick COMPONENTS Magick++ REQUIRED)
pkg_search_module(MAGICKXX REQUIRED Magick++ GraphicsMagick++)

pkg_search_module(EXIF REQUIRED libexif)

check_include_file(spnav.h HAVE_SPNAV)
find_library(SPNAV spnav)
if(SPNAV AND HAVE_SPNAV)
  message(STATUS "enabling Space Navigator support")
  set(SPNAV_CFLAGS_OTHER -DHAVE_SPACE_NAVIGATOR)
  set(SPNAV_LIBRARIES ${SPNAV})
  file(GLOB SPNAV_SOURCES src/spnav/space_navigator.cpp)
else()
  set(SPNAV_LIBRARIES)
  set(SPNAV_CFLAGS_OTHER)
endif()

check_include_file(/opt/vc/include/bcm_host.h HAVE_BCMHOST)
if(USE_OPENGLES2 OR HAVE_BCMHOST)
  set(OPENGL_LIBPATH /opt/vc/lib)
  set(OPENGL_INCLUDE_DIR /opt/vc/include)
  set(OPENGL_LIBRARIES GLESv2)
  set(OPENGL_CFLAGS_OTHER -DHAVE_OPENGLES2)
  link_directories(${OPENGL_LIBPATH})
else()
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
endif()

if(BUILD_GALAPIX_SDL)
  pkg_search_module(SDL2 REQUIRED sdl2)

  set(GALAPIX_SDL_DEPS_INCLUDE_DIRS
    ${SDL2_INCLUDE_DIRS})
  set(GALAPIX_SDL_DEPS_LIBRARIES
    ${SDL2_LIBRARIES})
endif()

if(BUILD_GALAPIX_GTK)
  pkg_search_module(GTKMM REQUIRED gtkmm-2.4)
  pkg_search_module(GLADEMM REQUIRED libglademm-2.4)
  pkg_search_module(GTKGLEXTMM REQUIRED gtkglextmm-1.2)

  set(GALAPIX_GTK_DEPS_INCLUDE_DIRS
    ${GTKMM_INCLUDE_DIRS}
    ${GLADEMM_INCLUDE_DIRS}
    ${GTKGLEXTMM_INCLUDE_DIRS})
  set(GALAPIX_GTK_DEPS_LIBRARIES
    ${GTKMM_LIBRARIES}
    ${GLADEMM_LIBRARIES}
    ${GTKGLEXTMM_LIBRARIES})
endif()

set(GALAPIX_DEPS_LIBRARIES
  libgalapix
  libgalapix_sdl
  logmich
  ${SPNAV_LIBRARIES}
  ${OPENGL_LIBRARIES}
  ${GLEW_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${PNG_LIBRARIES}
  ${JPEG_LIBRARIES}
  ${EXIF_LIBRARIES}
  ${CURL_LIBRARIES}
  ${MAGICKXX_LIBRARIES}
  ${MHASH_LIBRARIES}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SIGNALS_LIBRARY}
  ${CMAKE_THREAD_LIBS_INIT})

include_directories(src/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/)
include_directories(SYSTEM external/logmich/include/)
include_directories(SYSTEM external/glm/)
include_directories(SYSTEM external/googletest/googletest/include/)
include_directories(SYSTEM ${MAGICKXX_INCLUDE_DIRS})

# turn shader soure in a .hpp file
file(GLOB SHADER_SOURCE RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  src/shader/*.frag
  src/shader/*.vert)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/src/display)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/src/display/shader_vfs.hpp
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bin2h.py
  -o ${CMAKE_CURRENT_BINARY_DIR}/src/display/shader_vfs.hpp
  --namespace=shader_vfs --table
  ${SHADER_SOURCE}
  DEPENDS ${SHADER_SOURCE}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# compile logmich
file(GLOB LOGMICH_CXX_SOURCES
  external/logmich/src/log.cpp
  external/logmich/src/logger.cpp)
add_library(logmich STATIC ${LOGMICH_CXX_SOURCES})
set_target_properties(logmich PROPERTIES CXX_CLANG_TIDY "")

# compile libgalapix
file(GLOB LIBGALAPIX_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  src/archive/*.cpp
  src/database/*.cpp
  src/database/entries/*.cpp
  src/database/tables/*.cpp
  src/display/*.cpp
  src/galapix/*.cpp
  src/galapix/layouter/*.cpp
  src/server/*.cpp
  src/generator/*.cpp
  src/job/*.cpp
  src/jobs/*.cpp
  src/lisp/*.cpp
  src/math/*.cpp
  src/network/*.cpp
  src/plugins/*.cpp
  src/resource/*.cpp
  src/sqlite/*.cpp
  src/tools/*.cpp
  src/util/*.cpp)
list(APPEND LIBGALAPIX_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/src/display/shader_vfs.hpp)
add_library(libgalapix SHARED ${LIBGALAPIX_SOURCES})
target_compile_options(libgalapix PRIVATE ${WARNINGS_CXX_FLAGS})
set_target_properties(libgalapix PROPERTIES OUTPUT_NAME "galapix")
target_compile_options(libgalapix PRIVATE ${MAGICKXX_CFLAGS_OTHER})

install(TARGETS libgalapix
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# compile galapix.sdl
if(BUILD_GALAPIX_SDL)
  file(GLOB LIBGALAPIX_SDL_SOURCES src/sdl/*.cpp)
  list(APPEND LIBGALAPIX_SDL_SOURCES ${SPNAV_SOURCES})
  add_library(libgalapix_sdl SHARED ${LIBGALAPIX_SDL_SOURCES})
  target_compile_options(libgalapix_sdl PRIVATE ${WARNINGS_CXX_FLAGS})
  target_include_directories(libgalapix_sdl SYSTEM PRIVATE ${GALAPIX_SDL_DEPS_INCLUDE_DIRS})
  target_link_libraries(libgalapix_sdl ${GALAPIX_SDL_DEPS_LIBRARIES})
  target_compile_definitions(libgalapix_sdl PRIVATE -DGALAPIX_SDL)

  add_executable(galapix.sdl src/main/sdl_main.cpp)
  target_compile_options(galapix.sdl PRIVATE ${WARNINGS_CXX_FLAGS})
  target_compile_definitions(galapix.sdl PRIVATE -DGALAPIX_SDL ${SPNAV_CFLAGS_OTHER})
  target_link_libraries(galapix.sdl
    libgalapix_sdl
    ${GALAPIX_DEPS_LIBRARIES})

  install(TARGETS galapix.sdl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# compile galapix.gtk
if(BUILD_GALAPIX_GTK)
  file(GLOB LIBGALAPIX_GTK_SOURCES src/gtk/*.cpp)
  add_library(libgalapix_gtk SHARED ${LIBGALAPIX_GTK_SOURCES})
  target_compile_options(libgalapix_gtk PRIVATE ${WARNINGS_CXX_FLAGS})
  target_compile_definitions(libgalapix_gtk PRIVATE -DGALAPIX_GTK)
  target_include_directories(libgalapix_gtk SYSTEM PRIVATE
    ${GALAPIX_GTK_DEPS_INCLUDE_DIRS})
  target_link_libraries(libgalapix_gtk
    ${GALAPIX_DEPS_LIBRARIES}
    ${GALAPIX_GTK_DEPS_LIBRARIES})

  add_executable(galapix.gtk src/main/gtk_main.cpp)
  target_compile_options(galapix.gtk PRIVATE ${WARNINGS_CXX_FLAGS})
  target_compile_definitions(galapix.gtk PRIVATE -DGALAPIX_GTK)
  target_include_directories(galapix.gtk SYSTEM PRIVATE
    ${GALAPIX_GTK_DEPS_INCLUDE_DIRS})
  target_link_libraries(galapix.gtk
    libgalapix_gtk
    ${GALAPIX_DEPS_LIBRARIES}
    ${GALAPIX_GTK_DEPS_LIBRARIES})

  install(TARGETS libgalapix_gtk galapix.gtk
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

# compile extra
if(BUILD_EXTRA)
  file(GLOB EXTRA_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    extra/*.cpp)

  foreach(SRC ${EXTRA_SOURCES})
    get_filename_component(SRC_BASENAME ${SRC} NAME_WE)
    add_executable(${SRC_BASENAME} ${SRC})
    target_compile_options("${SRC_BASENAME}" PRIVATE ${WARNINGS_CXX_FLAGS})
    # use=(["galapix", "galapix_sdl"] + galapix_sdl_deps + galapix_deps))
    target_link_libraries(${SRC_BASENAME} ${GALAPIX_DEPS_LIBRARIES})
  endforeach(SRC)

  add_executable(imagescaler ${CMAKE_CURRENT_SOURCE_DIR}/extra/imagescaler/imagescaler.cpp)
  target_compile_options(imagescaler PRIVATE ${WARNINGS_CXX_FLAGS})
  target_link_libraries(imagescaler ${GALAPIX_DEPS_LIBRARIES})
endif()

# compile tests
if(BUILD_TESTS)
  # build gtest
  add_library(gtest STATIC ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/googletest/src/gtest-all.cc)
  target_include_directories(gtest SYSTEM PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/googletest/)

  # build gtest_main
  add_library(gtest_main STATIC ${CMAKE_CURRENT_SOURCE_DIR}/external/googletest/googletest/src/gtest_main.cc)

  # build automatic tests
  file(GLOB TEST_GALAPIX_SOURCES test/*_test.cpp)
  add_executable(test_galapix ${TEST_GALAPIX_SOURCES})
  target_compile_options(test_galapix PRIVATE ${WARNINGS_CXX_FLAGS})
  target_link_libraries(test_galapix
    gtest gtest_main
    ${GALAPIX_DEPS_LIBRARIES})

  # build uitest
  file(GLOB UITEST_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} external/uitest/src/*.cpp)
  add_library(uitest STATIC ${UITEST_SOURCES})
  target_include_directories(uitest SYSTEM PRIVATE external/uitest/include/)
  set_target_properties(uitest PROPERTIES CXX_CLANG_TIDY "")

  # build interactive tests
  file(GLOB UITEST_GALAPIX_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} uitest/*_test.cpp)
  add_executable(uitest_galapix ${UITEST_GALAPIX_SOURCES})
  target_compile_options(uitest_galapix PRIVATE ${WARNINGS_CXX_FLAGS})
  target_include_directories(uitest_galapix PRIVATE src/)
  target_include_directories(uitest_galapix SYSTEM PRIVATE
    external/uitest/include/)
  target_link_libraries(uitest_galapix uitest ${GALAPIX_DEPS_LIBRARIES})

  if(BUILD_GALAPIX_SDL)
    file(GLOB TEST_GALAPIX_SDL_SOURCES test/sdl/*_test.cpp)
    foreach(SRC ${TEST_GALAPIX_SDL_SOURCES})
      get_filename_component(SRC_BASENAME ${SRC} NAME_WE)
      add_executable(${SRC_BASENAME} ${SRC})
      target_compile_options(${SRC_BASENAME} PRIVATE ${WARNINGS_CXX_FLAGS})
      set_target_properties(${SRC_BASENAME} PROPERTIES OUTPUT_NAME "extra/${SRC_BASENAME}")
      target_link_libraries(${SRC_BASENAME} ${GALAPIX_DEPS_LIBRARIES})
      # use=(["galapix", "galapix_sdl"] + galapix_sdl_deps + galapix_deps))
    endforeach()
  endif()

  # add 'make test' target, use 'make test ARGS="-V"' or 'ctest -V' for verbose
  enable_testing()
  add_test(NAME test_galapix
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMAND test_galapix)
endif()

if(BUILD_BENCHMARKS)
  # build google-benchmark
  include_directories(${PROJECT_SOURCE_DIR}/src)
  file(GLOB BENCHMARK_SOURCES external/benchmark/src/*.cc)
  add_library(benchmark ${BENCHMARK_SOURCES})
  target_compile_definitions(benchmark PRIVATE -DHAVE_STD_REGEX)
  target_compile_options(benchmark PRIVATE -std=c++1y)
  target_include_directories(benchmark SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/benchmark/include)

  # build benchmarks
  file(GLOB BENCHMARKSOURCES benchmarks/*.cpp)
  foreach(SOURCE ${BENCHMARKSOURCES})
    get_filename_component(SOURCE_BASENAME ${SOURCE} NAME_WE)
    add_executable(${SOURCE_BASENAME} ${SOURCE})
    target_link_libraries(${SOURCE_BASENAME} benchmark ${GALAPIX_DEPS_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
    set_target_properties(${SOURCE_BASENAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks/")
    target_compile_options(${SOURCE_BASENAME} PRIVATE -std=c++1y ${WARNINGS_CXX_FLAGS})
  endforeach(SOURCE)
endif()

# EOF #
