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

cmake_minimum_required(VERSION 3.16...3.31)

# Version string (CLI --version / diagnostics):
#   Packaging (nix build): -DPROJECT_VERSION_FULL=0.3.0-dev.N+gHASH from flake.nix
#   Dev (nix develop / galapix-configure): VERSION + git, same scheme — NOT git-describe.
#     flake: "${versionBase}.${revCount}+g${shortRev or dirtyShortRev or "dirty"}"
#     cmake: "${VERSION}.${rev-list --count}+g$(rev-parse --short=7)"
#
# cmake --build re-runs configure when VERSION or .git/HEAD (or index) changes
# via CMAKE_CONFIGURE_DEPENDS, so the baked version tracks commits without a
# manual reconfigure.
if(DEFINED PROJECT_VERSION_FULL AND NOT PROJECT_VERSION_FULL STREQUAL "")
  string(STRIP "${PROJECT_VERSION_FULL}" PROJECT_VERSION_FULL)
else()
  file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION_FULL LIMIT_COUNT 1)
  string(STRIP "${PROJECT_VERSION_FULL}" PROJECT_VERSION_FULL)

  if(PROJECT_VERSION_FULL MATCHES "-dev")
    find_package(Git QUIET)
    set(_galapix_git_dir "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    if(Git_FOUND AND EXISTS "${_galapix_git_dir}")
      # Worktree: .git is a file "gitdir: …"
      if(NOT IS_DIRECTORY "${_galapix_git_dir}")
        file(READ "${_galapix_git_dir}" _galapix_gitfile)
        string(REGEX REPLACE "^gitdir:[ \t]*([^\r\n]+).*" "\\1" _galapix_git_dir
               "${_galapix_gitfile}")
        if(NOT IS_ABSOLUTE "${_galapix_git_dir}")
          set(_galapix_git_dir "${CMAKE_CURRENT_SOURCE_DIR}/${_galapix_git_dir}")
        endif()
      endif()

      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-list --count HEAD
        OUTPUT_VARIABLE _galapix_git_count
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _galapix_git_count_rc
      )
      execute_process(
        COMMAND "${GIT_EXECUTABLE}" -C "${CMAKE_CURRENT_SOURCE_DIR}" rev-parse --short=7 HEAD
        OUTPUT_VARIABLE _galapix_git_rev
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE _galapix_git_rev_rc
      )
      if(_galapix_git_count_rc EQUAL 0 AND _galapix_git_rev_rc EQUAL 0
         AND NOT _galapix_git_count STREQUAL "" AND NOT _galapix_git_rev STREQUAL "")
        set(PROJECT_VERSION_FULL
            "${PROJECT_VERSION_FULL}.${_galapix_git_count}+g${_galapix_git_rev}")
        message(STATUS "Dev version from git: ${PROJECT_VERSION_FULL}")
      endif()

      set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")
      if(EXISTS "${_galapix_git_dir}/HEAD")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
          "${_galapix_git_dir}/HEAD")
      endif()
      if(EXISTS "${_galapix_git_dir}/index")
        set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
          "${_galapix_git_dir}/index")
      endif()
    endif()
  endif()
endif()

# project() needs a numeric version; strip -dev / +git / similar suffixes.
string(REGEX MATCH "^[0-9]+(\\.[0-9]+)*" _project_version_cmake "${PROJECT_VERSION_FULL}")
if(NOT _project_version_cmake)
  set(_project_version_cmake "0.0.0")
endif()

# Single product binary: `galapix` (version lives in VERSION / --version).
project(galapix VERSION ${_project_version_cmake} LANGUAGES CXX C)

include(mk/cmake/TinyCMMC.cmake)

# Version header: only TUs that include it recompile on version bump.
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/galapix/version.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/src/galapix/version.hpp"
  @ONLY
)
message(STATUS "Galapix version: ${PROJECT_VERSION_FULL} (CMake PROJECT_VERSION=${PROJECT_VERSION})")

include(CheckIncludeFile)

option(USE_OPENGLES2 "Use OpenGLES2 instead of OpenGL" OFF)
option(THREADSANITIZER "Thread Sanitizer" OFF)
option(WITH_THUMTOO "Use thumtoo as tile cache backend" OFF)
set(THUMTOO_DIR "" CACHE PATH "thumtoo source tree or install prefix")

if(THREADSANITIZER)
  add_compile_options(-fsanitize=thread)
endif()

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(CURL REQUIRED)

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
else()
  set(OpenGL_GL_PREFERENCE "GLVND")
  find_package(OpenGL REQUIRED)
  # GL entry points via glad (wstdisplay); do not link GLEW.
endif()

function(build_dependencies)
  if(BUILD_TESTS)
    set(BUILD_TESTS OFF)
    tinycmmc_find_dependency(uitest)
  endif()
  set(BUILD_TESTS OFF)



  tinycmmc_find_dependency(geom)
  tinycmmc_find_dependency(logmich)
  tinycmmc_find_dependency(sexp)
  tinycmmc_find_dependency(strut)
  tinycmmc_find_dependency(surf)
  tinycmmc_find_dependency(prio)
  tinycmmc_find_dependency(wstdisplay)
endfunction()
build_dependencies()

# compile libgalapix
file(GLOB LIBGALAPIX_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
  src/galapix/*.cpp
  src/galapix/layouter/*.cpp
  src/job/*.cpp
  src/jobs/*.cpp
  src/lisp/*.cpp
  src/math/*.cpp
  src/network/*.cpp
  src/plugins/*.cpp
  src/surface/*.cpp
  src/tools/*.cpp
  src/util/*.cpp)
add_library(libgalapix SHARED ${LIBGALAPIX_SOURCES})
set_target_properties(libgalapix PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
target_compile_options(libgalapix PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
target_compile_definitions(libgalapix PUBLIC
  -DPROJECT_NAME="${PROJECT_NAME}")
target_include_directories(libgalapix PUBLIC
  src/
  ${CMAKE_CURRENT_BINARY_DIR}/src/)
target_link_libraries(libgalapix PUBLIC
  prio::prio
  surf::surf
  strut::strut
  logmich::logmich
  sexp::sexp
  geom::geom
  wstdisplay::wstdisplay
  PkgConfig::MAGICKXX
  PkgConfig::SIGCXX
  ${GALAPIX_SPNAV_LIBRARIES}
  OpenGL::GL
  OpenGL::GLU
  CURL::libcurl
  Threads::Threads)

install(TARGETS libgalapix
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})


# Optional thumtoo tile backend (Galapix-compatible 256² JPEG pyramid).
if(WITH_THUMTOO)
  if(NOT THUMTOO_DIR)
    message(FATAL_ERROR "WITH_THUMTOO requires THUMTOO_DIR (source or prefix)")
  endif()
  if(EXISTS "${THUMTOO_DIR}/CMakeLists.txt")
    # Build against a thumtoo source checkout.
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(THUMTOO_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(THUMTOO_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    add_subdirectory("${THUMTOO_DIR}" "${CMAKE_BINARY_DIR}/thumtoo" EXCLUDE_FROM_ALL)
    if(TARGET thumtoo)
      set(THUMTOO_TARGET thumtoo)
    elseif(TARGET thumtoo::thumtoo)
      set(THUMTOO_TARGET thumtoo::thumtoo)
    else()
      message(FATAL_ERROR "thumtoo CMake target not found in ${THUMTOO_DIR}")
    endif()
  else()
    find_path(THUMTOO_INCLUDE_DIR thumtoo/client.hpp
      HINTS "${THUMTOO_DIR}/include" "${THUMTOO_DIR}")
    find_library(THUMTOO_LIBRARY NAMES thumtoo
      HINTS "${THUMTOO_DIR}/lib" "${THUMTOO_DIR}")
    if(NOT THUMTOO_INCLUDE_DIR OR NOT THUMTOO_LIBRARY)
      message(FATAL_ERROR "Could not find thumtoo in ${THUMTOO_DIR}")
    endif()
    add_library(thumtoo_imported UNKNOWN IMPORTED)
    set_target_properties(thumtoo_imported PROPERTIES
      IMPORTED_LOCATION "${THUMTOO_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES "${THUMTOO_INCLUDE_DIR}")
    set(THUMTOO_TARGET thumtoo_imported)
  endif()

  target_sources(libgalapix PRIVATE
    src/thumtoo/thumtoo_callback_queue.cpp
    src/thumtoo/thumtoo_tile_provider.cpp
    src/thumtoo/thumtoo_uri.cpp)
  target_compile_definitions(libgalapix PUBLIC HAVE_THUMTOO=1)
  target_link_libraries(libgalapix PUBLIC ${THUMTOO_TARGET})
  message(STATUS "thumtoo tile backend enabled (${THUMTOO_DIR})")
endif()


# --- Tool icons: SVG → PNG at build time (ImGui loads PNG via SDL_image) ---
set(GALAPIX_ICON_SVG_DIR "${CMAKE_CURRENT_SOURCE_DIR}/data/icons/svg")
set(GALAPIX_ICON_PNG_DIR "${CMAKE_BINARY_DIR}/share/galapix/icons/hicolor/24x24/actions")
file(MAKE_DIRECTORY "${GALAPIX_ICON_PNG_DIR}")

set(_galapix_icon_names
  pan_tool zoom_rect_tool grid_tool move_tool
  grid grid_pin
  layout_regular layout_tight layout_random
  layout_solve layout_spiral layout_vertical
  status help)

find_program(GALAPIX_RSVG_CONVERT NAMES rsvg-convert)
find_program(GALAPIX_IM_CONVERT NAMES magick convert)

set(GALAPIX_ICON_PNGS "")
foreach(_name ${_galapix_icon_names})
  set(_svg "${GALAPIX_ICON_SVG_DIR}/${_name}.svg")
  set(_png "${GALAPIX_ICON_PNG_DIR}/${_name}.png")
  if(NOT EXISTS "${_svg}")
    message(FATAL_ERROR "Missing tool icon SVG: ${_svg}")
  endif()
  if(GALAPIX_RSVG_CONVERT)
    add_custom_command(
      OUTPUT "${_png}"
      COMMAND "${GALAPIX_RSVG_CONVERT}" -w 24 -h 24 -f png -o "${_png}" "${_svg}"
      DEPENDS "${_svg}"
      COMMENT "Rasterize ${_name}.svg (rsvg-convert)"
      VERBATIM)
  elseif(GALAPIX_IM_CONVERT)
    add_custom_command(
      OUTPUT "${_png}"
      COMMAND "${GALAPIX_IM_CONVERT}" -background none -density 192 "${_svg}" -resize 24x24 "PNG32:${_png}"
      DEPENDS "${_svg}"
      COMMENT "Rasterize ${_name}.svg (ImageMagick)"
      VERBATIM)
  else()
    message(FATAL_ERROR
      "Need rsvg-convert or ImageMagick convert to build tool icons from SVG")
  endif()
  list(APPEND GALAPIX_ICON_PNGS "${_png}")
endforeach()

add_custom_target(galapix_icons ALL DEPENDS ${GALAPIX_ICON_PNGS})

# String path for ImGui icon loader (avoid fragile -DSTRING= macros).
set(GALAPIX_DEFAULT_DATADIR "${CMAKE_BINARY_DIR}/share/galapix")
configure_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/src/app/galapix_paths.hpp.in"
  "${CMAKE_CURRENT_BINARY_DIR}/src/app/galapix_paths.hpp"
  @ONLY)
if(GALAPIX_RSVG_CONVERT)
  message(STATUS "Tool icons: SVG→PNG via ${GALAPIX_RSVG_CONVERT}")
else()
  message(STATUS "Tool icons: SVG→PNG via ${GALAPIX_IM_CONVERT}")
endif()

# Application shell (SDL2 window + ImGui chrome). Always built.
pkg_search_module(SDL2 REQUIRED sdl2 IMPORTED_TARGET)
pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image IMPORTED_TARGET)

file(GLOB LIBGALAPIX_APP_SOURCES src/app/*.cpp)
list(APPEND LIBGALAPIX_APP_SOURCES ${GALAPIX_SPNAV_SOURCES})
# Dear ImGui (vendored) — chrome overlay for the viewer.
set(IMGUI_SOURCES
  external/imgui/imgui.cpp
  external/imgui/imgui_draw.cpp
  external/imgui/imgui_tables.cpp
  external/imgui/imgui_widgets.cpp
  external/imgui/backends/imgui_impl_sdl2.cpp
  external/imgui/backends/imgui_impl_opengl3.cpp)
list(APPEND LIBGALAPIX_APP_SOURCES ${IMGUI_SOURCES})
add_library(libgalapix_app STATIC ${LIBGALAPIX_APP_SOURCES})
target_compile_options(libgalapix_app PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
# ImGui is third-party; do not apply project warning flags to it.
set_source_files_properties(${IMGUI_SOURCES} PROPERTIES COMPILE_FLAGS "-w")
target_include_directories(libgalapix_app PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
  ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
  ${CMAKE_CURRENT_BINARY_DIR}/src)
target_link_libraries(libgalapix_app PUBLIC
  libgalapix
  PkgConfig::SDL2
  PkgConfig::SDL2_IMAGE)

add_executable(galapix src/main/main.cpp)
add_dependencies(galapix galapix_icons)
target_compile_options(galapix PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
target_compile_definitions(galapix PRIVATE ${GALAPIX_SPNAV_DEFINES})
target_link_libraries(galapix PRIVATE libgalapix_app)

install(TARGETS galapix
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})


# compile extra
if(BUILD_EXTRA)
  file(GLOB EXTRA_SOURCES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    extra/*.cpp)

  foreach(SRC ${EXTRA_SOURCES})
    get_filename_component(SRC_BASENAME ${SRC} NAME_WE)
    add_executable(${SRC_BASENAME} ${SRC})
    target_compile_options("${SRC_BASENAME}" PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
    # use=(["galapix", "galapix_app"] + galapix_app_deps + galapix_deps))
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

  file(GLOB TEST_GALAPIX_APP_SOURCES test/app/*_test.cpp test/sdl/*_test.cpp)
  foreach(SRC ${TEST_GALAPIX_APP_SOURCES})
    get_filename_component(SRC_BASENAME ${SRC} NAME_WE)
    add_executable(${SRC_BASENAME} ${SRC})
    target_compile_options(${SRC_BASENAME} PRIVATE ${TINYCMMC_WARNINGS_CXX_FLAGS})
    set_target_properties(${SRC_BASENAME} PROPERTIES OUTPUT_NAME "extra/${SRC_BASENAME}")
    target_link_libraries(${SRC_BASENAME} libgalapix)
  endforeach()

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

# Tool icons: SVG sources + build-time PNG (see galapix_icons target)
install(DIRECTORY data/icons/svg
  DESTINATION ${CMAKE_INSTALL_DATADIR}/galapix/icons)
install(DIRECTORY "${GALAPIX_ICON_PNG_DIR}/"
  DESTINATION ${CMAKE_INSTALL_DATADIR}/galapix/icons/hicolor/24x24/actions
  OPTIONAL)

# EOF #
