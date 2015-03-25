# Galapix - an image viewer for large image collections
# Copyright (C) 2015 Ingo Ruhnke <grumbel@gmx.de>
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

include(FindOpenGL)
include(FindGLEW)
include(FindPkgConfig)
include(FindBoost)

find_package(Threads)
find_package(Boost COMPONENTS system filesystem signals REQUIRED)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(EXIF REQUIRED libexif)
pkg_search_module(SQLITE3 REQUIRED sqlite3)
pkg_search_module(MHASH REQUIRED mhash)
pkg_search_module(CURL REQUIRED libcurl)
pkg_search_module(MAGICKXX REQUIRED Magick++)
pkg_search_module(PNG REQUIRED libpng)
find_library(JPEG_LIBRARIES NAMES jpeg)

set(BUILD_GALAPIX_SDL 1 CACHE BOOL "Build galapix.sdl")
set(BUILD_GALAPIX_GTK 0 CACHE BOOL "Build galapix.gtk")

if(BUILD_GALAPIX_GTK)
  pkg_search_module(GTKMM REQUIRED gtkmm-2.4)
  pkg_search_module(GLADEMM REQUIRED libglademm-2.4)
  pkg_search_module(GTKGLEXTMM REQUIRED gtkglextmm-1.2)
endif()

find_library(SPNAV spnav)
if(SPNAV)
  # conf.check_cxx(header_name="spnav.h", mandatory=False):
  # conf.env["DEFINES_SPNAV"] = ['HAVE_SPACE_NAVIGATOR=1']
  # conf.env["HAVE_SPNAV"] = 1
endif()

if(USE_OPENGLES2)
  # conf.end_msg("yes")
  # conf.env["LIB_OPENGL"] = ['GLESv2']
  # conf.env["LIBPATH_OPENGL"] = ["/opt/vc/lib"]
  # conf.env["INCLUDES_OPENGL"] = ["/opt/vc/include"]
  # conf.env["DEFINES_OPENGL"] = ['HAVE_OPENGLES2']
else()
  # conf.end_msg("no")
  # conf.check_cfg(package='gl', args=['--cflags', '--libs'])
  # conf.check_cfg(package='glew', args=['--cflags', '--libs'])

  # # build OPENGL uselib from GL and GLEW
  # for lib in ["GL", "GLEW"]:
  #     for t in ["LIB", "LIBPATH", "INCLUDES", "DEFINES"]:
  #         conf.env.append_value(t + "_" + "OPENGL", conf.env[t + "_" + lib])
endif()

set(PROFILE_CXXFLAGS "-std=c++1y -O2 -g3 -pg")
set(PROFILE_LINKFLAGS "-pg")

set(DEBUG_CXXFLAGS "-O0 -g3")

set(RELEASE_CXXFLAGS "-O3 -s")

set(DEVELOPER_CXXFLAGS "\
-pedantic -Wall -Wextra -Wno-c++0x-compat -Wnon-virtual-dtor -Weffc++ \
-Wconversion -Werror -Wshadow -Wcast-qual -Winit-self -Wno-unused-parameter")
# -Winline
# -Wfloat-equal
# -Wunreachable-code

# gr.add_option('--build-galapix-gtk', action='store_true', default=False, help='Build galapix.gtk')
# gr.add_option('--build-galapix-sdl', action='store_true', default=True, help='Build galapix.sdl')
# gr.add_option('--build-tests', action='store_true', default=False, help='Build tests')
# gr.add_option('--build-extra', action='store_true', default=False, help='Build extra')
# gr.add_option('--developer', action='store_true', default=False, help='Switch on extra debug info, warnings and verbosity')
# gr.add_option('--debug', action='store_true', default=False, help='Switch on extra debug info')
# gr.add_option('--release', action='store_true', default=False, help='Switch on optimizations')

# def use_opengles2():
#     # check if it's a Raspberry Pi
#     return platform.machine() == 'armv6l'

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")

set(OPTIONAL_SOURCES)

if(HAVE_SPNAV)
  file(GLOB SPNAV_SOURCES src/spnav/space_navigator.cpp)
  list(APPEND OPTIONAL_SOURCES ${SPNAV_SOURCES})
endif()

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
  src/util/*.cpp
  src/display/shader_vfs.hpp)
list(APPEND LIBGALAPIX_SOURCES ${CMAKE_CURRENT_BINARY_DIR}/src/display/shader_vfs.hpp)

# galapix_deps = ["WARNINGS",
# "pthread", "glm", "logmich", "SPNAV",
# "MAGICKXX", "LIBPNG", "LIBEXIF", "JPEG", "LIBCURL", "MHASH",
# "SQLITE3", "OPENGL", "BOOST_FILESYSTEM"]

# galapix_gtk_deps = ["GTKMM", "GLADEMM", "GTKGLEXTMM", "SDL2"]
# galapix_sdl_deps = ["SDL2"]

# # build 'galapix_vfs.hpp'
# def vfs_builder(task):
#   build_bin2h(task.outputs, task.inputs, "shader_vfs")
#   return None  # success

# bld(target="src/display/shader_vfs.hpp",
#     source=glob("src/shader/*.frag") + glob("src/shader/*.vert"),
#     rule=vfs_builder)

include_directories(src/)
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/)
include_directories(SYSTEM external/logmich/include/)
include_directories(SYSTEM external/glm-0.9.6.1/)
include_directories(SYSTEM ${MAGICKXX_INCLUDE_DIRS})

 get_cmake_property(_variableNames VARIABLES)
 foreach (_variableName ${_variableNames})
   message(STATUS "${_variableName}=${${_variableName}}")
 endforeach()

file(GLOB LOGMICH_CXX_SOURCES
  external/logmich/src/log.cpp
  external/logmich/src/logger.cpp)

add_library(logmich STATIC ${LOGMICH_CXX_SOURCES})

add_library(libgalapix STATIC ${LIBGALAPIX_SOURCES})
set_target_properties(libgalapix PROPERTIES OUTPUT_NAME "galapix")
target_compile_options(libgalapix PUBLIC ${MAGICKXX_CFLAGS_OTHER})

file(GLOB LIBGALAPIX_SDL_SOURCES src/sdl/*.cpp)
file(GLOB LIBGALAPIX_GTK_SOURCES src/gtk/*.cpp)

if(BUILD_GALAPIX_SDL)
  add_library(libgalapix_sdl STATIC ${LIBGALAPIX_SDL_SOURCES})
  target_include_directories(libgalapix_sdl PUBLIC ${SDL2_INCLUDE_DIRS})
  target_link_libraries(libgalapix_sdl ${SDL2_LIBRARIES})
  target_compile_definitions(libgalapix_sdl PUBLIC -DGALAPIX_SDL)

  add_executable(galapix.sdl src/main/sdl_main.cpp)
  target_compile_definitions(galapix.sdl PUBLIC -DGALAPIX_SDL)
  target_link_libraries(galapix.sdl
    libgalapix
    libgalapix_sdl
    logmich
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
endif()

if(BUILD_GALAPIX_GTK)
  add_library(libgalapix_gtk STATIC ${LIBGALAPIX_GTK_SOURCES})

  add_executable(galapix.gtk src/main/gtk_main.cpp)
  target_compile_definitions(galapix.gtk -DGALAPIX_GTK)
  target_link_libraries(galapix.gtk libgalapix_gtk)
endif()

# if bld.env.build_extra:
#     for filename in glob("extra/*.cpp"):
#         bld.program(target=filename[:-4],
#                     source=filename,
#                     includes=["src/"],
#                     use=(["galapix", "galapix_sdl"] + galapix_sdl_deps + galapix_deps))
#     bld.program(target="extra/imagescaler/imagescaler",
#                 source="extra/imagescaler/imagescaler.cpp",
#                 includes=["src/"],
#                 use=(["galapix"] + galapix_deps))

# if bld.env.build_tests:
#     # build gtest
#     bld.stlib(target="gtest",
#               source=["external/googletest/src/gtest-all.cc"],
#               includes=["external/googletest/include/",
#                         "external/googletest/"])

#     bld.stlib(target="gtest_main",
#               source=["external/googletest/src/gtest_main.cc"],
#               includes=["external/googletest/include/",
#                         "external/googletest/"])

#     # build automatic tests
#     bld.program(target="test_galapix",
#                 source=glob("test/*_test.cpp"),
#                 includes=["src/"],
#                 cxxflags=["-isystem", bld.path.find_dir("external/googletest/include/").abspath()],
#                 use=(["gtest", "gtest_main"] +
#                      ["galapix"] + galapix_deps))

#     # build interactive tests
#     # build gtest
#     bld.stlib(target="uitest",
#               source=glob("external/uitest/src/*.cpp"),
#               includes=["external/uitest/include/"],
#               export_includes=["external/uitest/include/"])

#     bld.program(target="uitest_galapix",
#                 source=glob("uitest/*_test.cpp"),
#                 includes=["src/"],
#                 use=(["uitest", "galapix"] + galapix_deps))

#     if bld.env.build_galapix_sdl:
#         for filename in glob("uitest/sdl/*_test.cpp"):
#             bld.program(target=filename[:-4],
#                         source=[filename] + optional_sources,
#                         includes=["src/"],
#                         use=(["galapix", "galapix_sdl"] + galapix_sdl_deps + galapix_deps))

# EOF #
