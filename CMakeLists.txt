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

cmake_minimum_required(VERSION 3.6)
project(galapix-0.3)

include(mk/cmake/TinyCMMC.cmake)

include(CheckIncludeFile)

option(BUILD_GALAPIX_SDL "Build galapix.sdl" ON)
option(BUILD_GALAPIX_GTK "Build galapix.gtk" OFF)
option(USE_OPENGLES2 "Use OpenGLES2 instead of OpenGL" OFF)
option(THREADSANITIZER "Thread Sanitizer" OFF)

if(THREADSANITIZER)
  add_compile_options(-fsanitize=thread)
endif()

find_package(PkgConfig REQUIRED)
find_package(Python)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)
find_package(fmt REQUIRED)
find_package(OpenSSL COMPONENTS Crypto REQUIRED)

find_package(spnav)
if(SPNAV_FOUND)
  message(STATUS "Enabling Space Navigator support")
  set(GALAPIX_SPNAV_LIBRARIES spnav::spnav)
  set(GALAPIX_SPNAV_DEFINES -DHAVE_SPACE_NAVIGATOR)
  file(GLOB GALAPIX_SPNAV_SOURCES src/spnav/space_navigator.cpp)
else()
  message(WARNING "Disabling Space Navigator support")
endif()

# Not using find_package(), see:
# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=776832
# find_package(ImageMagick COMPONENTS Magick++ REQUIRED)
pkg_search_module(MAGICKXX REQUIRED Magick++ GraphicsMagick++ IMPORTED_TARGET)

pkg_search_module(SQLITE3 REQUIRED sqlite3 IMPORTED_TARGET)
pkg_search_module(SIGCXX REQUIRED sigc++-2.0 IMPORTED_TARGET)

check_include_file(/opt/vc/include/bcm_host.h HAVE_BCMHOST)
if(USE_OPENGLES2 OR HAVE_BCMHOST)
  message(STATUS "Using OpenGLES2")
  find_library(OPENGL_LIBRARIES GLESv2 ${OPENGL_LIBPATH})

  add_library(OpenGL::GL INTERFACE IMPORTED)
  target_link_libraries(OpenGL::GL INTERFACE ${OPENGL_LIBRARIES})
  target_compile_definitions(OpenGL::GL INTERFACE HAVE_OPENGLES2)
  if(HAVE_BCMHOST)
    target_include_directories(OpenGL::GL INTERFACE /opt/vc/include)
    target_link_directories(OpenGL::GL INTERFACE /opt/vc/lib)
  endif()
  add_library(OpenGL::GLU INTERFACE IMPORTED)
  add_library(GLEW::GLEW INTERFACE IMPORTED)
else()
  set(OpenGL_GL_PREFERENCE "GLVND")
  find_package(OpenGL REQUIRED)
  find_package(GLEW REQUIRED)
endif()

function(build_dependencies)
  if(BUILD_TESTS)
    set(BUILD_TESTS OFF)
    tinycmmc_find_dependency(uitest)
  endif()
  set(BUILD_TESTS OFF)

  find_package(SQLiteCpp)
  if(NOT TARGET SQLiteCpp)
    add_subdirectory(external/SQLiteCpp/ EXCLUDE_FROM_ALL)
    set_target_properties(SQLiteCpp PROPERTIES CXX_CLANG_TIDY "")
  endif()

  find_package(EnTT)
  if(NOT TARGET EnTT::EnTT)
    add_library(EnTT::EnTT INTERFACE IMPORTED)
    target_include_directories(entt INTERFACE external/entt/single_include/)
  endif()

  tinycmmc_find_dependency(geom)
  tinycmmc_find_dependency(logmich)
  tinycmmc_find_dependency(sexp)
  tinycmmc_find_dependency(strut)
  tinycmmc_find_dependency(surf)
  tinycmmc_find_dependency(arxp)
  tinycmmc_find_dependency(prio)
  tinycmmc_find_dependency(wstdisplay)
endfunction()
build_dependencies()

# compile libgalapix
file(GLOB LIBGALAPIX_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  src/database/*.cpp
  src/database/entries/*.cpp
  src/database/tables/*.cpp
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
  src/surface/*.cpp
  src/sqlite/*.cpp
  src/tools/*.cpp
  src/util/*.cpp)
add_library(libgalapix SHARED ${LIBGALAPIX_SOURCES})
set_target_properties(libgalapix PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
target_compile_options(libgalapix PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
target_compile_definitions(libgalapix PUBLIC
  -DPROJECT_VERSION="${PROJECT_VERSION}"
  -DPROJECT_NAME="${PROJECT_NAME}")
target_include_directories(libgalapix PUBLIC
  src/
  ${CMAKE_CURRENT_BINARY_DIR}/src/)
target_link_libraries(libgalapix PUBLIC
  prio::prio
  arxp::arxp
  surf::surf
  strut::strut
  logmich::logmich
  sexp::sexp
  EnTT::EnTT
  geom::geom
  wstdisplay::wstdisplay
  SQLiteCpp
  fmt::fmt
  PkgConfig::MAGICKXX
  PkgConfig::SIGCXX
  ${GALAPIX_SPNAV_LIBRARIES}
  OpenSSL::Crypto
  OpenGL::GL
  OpenGL::GLU
  GLEW::GLEW
  PkgConfig::SQLITE3
  CURL::libcurl
  Threads::Threads)

install(TARGETS libgalapix
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(BUILD_GALAPIX_SDL)
  pkg_search_module(SDL2 REQUIRED sdl2 IMPORTED_TARGET)

  file(GLOB LIBGALAPIX_SDL_SOURCES src/sdl/*.cpp)
  list(APPEND LIBGALAPIX_SDL_SOURCES ${GALAPIX_SPNAV_SOURCES})
  add_library(libgalapix_sdl STATIC ${LIBGALAPIX_SDL_SOURCES})
  target_compile_options(libgalapix_sdl PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  target_link_libraries(libgalapix_sdl PUBLIC
    libgalapix
    PkgConfig::SDL2)
  target_compile_definitions(libgalapix_sdl PRIVATE -DGALAPIX_SDL)

  add_executable(galapix.sdl src/main/sdl_main.cpp)
  set_target_properties(galapix.sdl PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.sdl")
  target_compile_options(galapix.sdl PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  target_compile_definitions(galapix.sdl PRIVATE -DGALAPIX_SDL ${GALAPIX_SPNAV_DEFINES})
  target_link_libraries(galapix.sdl PRIVATE libgalapix_sdl)

  install(TARGETS galapix.sdl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

if(BUILD_GALAPIX_GTK)
  pkg_search_module(GTKMM REQUIRED gtkmm-3.0 IMPORTED_TARGET)

  file(GLOB LIBGALAPIX_GTK_SOURCES src/gtk/*.cpp)
  add_library(libgalapix_gtk SHARED ${LIBGALAPIX_GTK_SOURCES})
  target_compile_options(libgalapix_gtk PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  target_compile_definitions(libgalapix_gtk PRIVATE -DGALAPIX_GTK)
  target_link_libraries(libgalapix_gtk PUBLIC
    libgalapix
    PkgConfig::GTKMM)

  add_executable(galapix.gtk src/main/gtk_main.cpp)
  set_target_properties(galapix.gtk PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.gtk")
  target_compile_options(galapix.gtk PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  target_compile_definitions(galapix.gtk PRIVATE -DGALAPIX_GTK)
  target_link_libraries(galapix.gtk PRIVATE libgalapix_gtk)

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
    target_compile_options("${SRC_BASENAME}" PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
    # use=(["galapix", "galapix_sdl"] + galapix_sdl_deps + galapix_deps))
    target_link_libraries(${SRC_BASENAME} libgalapix)
  endforeach(SRC)

  add_executable(imagescaler ${CMAKE_CURRENT_SOURCE_DIR}/extra/imagescaler/imagescaler.cpp)
  target_compile_options(imagescaler PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  target_link_libraries(imagescaler libgalapix)
endif()

# compile tests
if(BUILD_TESTS)
  find_package(GTest REQUIRED)

  # build automatic tests
  file(GLOB TEST_GALAPIX_SOURCES test/*_test.cpp)
  add_executable(test_galapix ${TEST_GALAPIX_SOURCES})
  target_compile_options(test_galapix PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  target_link_libraries(test_galapix
    GTest::GTest
    GTest::Main
    libgalapix)

  # build interactive tests
  file(GLOB UITEST_GALAPIX_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} uitest/*_test.cpp)
  add_executable(uitest_galapix ${UITEST_GALAPIX_SOURCES})
  target_compile_options(uitest_galapix PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  target_include_directories(uitest_galapix PRIVATE src/)
  target_link_libraries(uitest_galapix
    libgalapix
    uitest::uitest
    uitest::uitest_main)

  if(BUILD_GALAPIX_SDL)
    file(GLOB TEST_GALAPIX_SDL_SOURCES test/sdl/*_test.cpp)
    foreach(SRC ${TEST_GALAPIX_SDL_SOURCES})
      get_filename_component(SRC_BASENAME ${SRC} NAME_WE)
      add_executable(${SRC_BASENAME} ${SRC})
      target_compile_options(${SRC_BASENAME} PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
      set_target_properties(${SRC_BASENAME} PROPERTIES OUTPUT_NAME "extra/${SRC_BASENAME}")
      target_link_libraries(${SRC_BASENAME} libgalapix)
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
  find_package(benchmark REQUIRED)

  # build benchmarks
  file(GLOB BENCHMARKSOURCES benchmarks/*.cpp)
  foreach(SOURCE ${BENCHMARKSOURCES})
    get_filename_component(SOURCE_BASENAME ${SOURCE} NAME_WE)
    add_executable(${SOURCE_BASENAME} ${SOURCE})
    target_link_libraries(${SOURCE_BASENAME}
      libgalapix
      benchmark::benchmark)
    set_target_properties(${SOURCE_BASENAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/benchmarks/")
    target_compile_options(${SOURCE_BASENAME} PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
  endforeach(SOURCE)
endif()

install(FILES
  galapix.svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)

install(FILES
  galapix.desktop
  DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)

# EOF #
