#! /usr/bin/env python
# -*- coding: utf-8 -*-

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

from glob import glob
import platform

APPNAME = 'galapix'
VERSION = '0.3.0'

profile_cxxflags = [ "-std=c++1y", "-O2", "-g3", "-pg" ]
profile_linkflags = [ "-pg" ]

debug_cxxflags = [ "-O0", "-g3" ]

release_cxxflags = [ "-O3", "-s" ]

warning_cxxflags = [
    # "-ansi",
    "-pedantic",
    "-Wall",
    "-Wextra",
    "-Wno-c++0x-compat",
    "-Wnon-virtual-dtor",
    "-Weffc++",
    "-Wconversion",
    "-Werror",
    "-Wshadow",
    "-Wcast-qual",
    "-Winit-self", # only works with >= -O1
    "-Wno-unused-parameter",
    # "-Winline",
    # "-Wfloat-equal",
    # "-Wunreachable-code",
]


def use_opengles2():
    # check if it's a Raspberry Pi
    return platform.machine() == 'armv6l'


def includes_to_isystem(ctx):
    """fudge around to turn -I flags into -isystem flags"""

    for key in ctx.env.keys():
        if key.startswith("INCLUDES_"):
            uselib = key[len("INCLUDES_"):]
            if type(ctx.env['INCLUDES_' + uselib]) == str:
                ctx.env['INCLUDES_' + uselib] = [ctx.env['INCLUDES_' + uselib]]
            cxxflags = [i for sublist in [('-isystem', p) for p in ctx.env['INCLUDES_' + uselib]] for i in sublist]
            ctx.env.append_value('CXXFLAGS_' + uselib, cxxflags)
            del ctx.env['INCLUDES_' + uselib]


def options(opt):
    opt.load('compiler_cxx')

    opt.load('compiler_cxx boost')

    gr = opt.add_option_group('Galapix options')
    gr.add_option('--build-galapix-gtk', action='store_true', default=False, help='Build galapix.gtk')
    gr.add_option('--build-galapix-sdl', action='store_true', default=True, help='Build galapix.sdl')
    gr.add_option('--build-tests', action='store_true', default=False, help='Build tests')
    gr.add_option('--build-extra', action='store_true', default=False, help='Build extra')
    gr.add_option('--developer', action='store_true', default=False, help='Switch on extra warnings and verbosity')


def configure(conf):
    conf.load('g++')

    conf.load('compiler_cxx boost')

    # this one fails to find the libraries on Raspberry Pi
    # conf.check_boost(lib='system filesystem')

    conf.check_cxx(lib=["boost_system", "boost_filesystem"],
                   header_name="boost/filesystem.hpp",
                   uselib_store="BOOST_FILESYSTEM")
    conf.check_cxx(header_name="boost/signals2.hpp", uselib_store="BOOST_SIGNAL2")

    conf.check_cfg(atleast_pkgconfig_version='0.0.0')
    conf.check_cfg(package='sdl2', args=['--cflags', '--libs'])
    conf.check_cfg(package='libexif', args=['--cflags', '--libs'])
    conf.check_cfg(package='sqlite3', args=['--cflags', '--libs'])
    conf.check_cfg(package='mhash', args=['--cflags', '--libs'])
    conf.check_cfg(package='libcurl', args=['--cflags', '--libs'])
    conf.check_cfg(package='Magick++', args=['--cflags', '--libs'], uselib_store="MAGICKXX")
    conf.check_cfg(package='libpng', args=['--cflags', '--libs'])
    conf.check_cxx(lib='jpeg')

    if conf.options.build_galapix_gtk:
        conf.check_cfg(package='gtkmm-2.4', args=['--cflags', '--libs'], uselib_store='GTKMM')
        conf.check_cfg(package='libglademm-2.4', args=['--cflags', '--libs'], uselib_store='GLADEMM')
        conf.check_cfg(package='gtkglextmm-1.2', args=['--cflags', '--libs'], uselib_store='GTKGLEXTMM')

    if use_opengles2():
        conf.env["OPENGL_LIBS"] = ['GLESv2']
        conf.env["OPENGL_LIBPATH"] = ["/opt/vc/lib"]
        conf.env["OPENGL_INCLUDES"] = ["/opt/vc/include"]
        conf.env["OPENGL_DEFINES"] = ['HAVE_OPENGLES2']
    else:
        conf.check_cfg(package='gl', args=['--cflags', '--libs'])
        conf.check_cfg(package='glew', args=['--cflags', '--libs'])

        # build OPENGL uselib from GL and GLEW
        for lib in ["GL", "GLEW"]:
            for t in ["LIB", "LIBPATH", "INCLUDES", "DEFINES"]:
                conf.env.append_value(t + "_" + "OPENGL", conf.env[t + "_" + lib])

    if conf.check_cxx(lib="spnav", mandatory=False) and \
       conf.check_cxx(header_name="spnav.h", mandatory=False):
        conf.env["DEFINES_SPNAV"] = ['HAVE_SPACE_NAVIGATOR=1']
        conf.env["HAVE_SPNAV"] = 1

    # pthread pseudo uselib
    conf.env["CFLAGS_pthread"] = ["-pthread"]
    conf.env["CXXFLAGS_pthread"] = ["-pthread"]
    conf.env["LINKFLAGS_pthread"] = ["-pthread"]

    # glm pseudo uselib
    conf.env["CXXFLAGS_glm"] = ["-isystem", conf.path.find_dir("external/glm-0.9.6.1/").abspath()]

    includes_to_isystem(conf)
    conf.env.append_value("CXXFLAGS", ["-std=c++1y"])

    if conf.options.developer:
        conf.env["CXXFLAGS_WARNINGS"] = warning_cxxflags

    print

    print "Build Galapix.SDL:", conf.options.build_galapix_sdl
    print "Build Galapix.Gtk:", conf.options.build_galapix_gtk
    print "Build Tests:      ", conf.options.build_tests
    print "Build Extra:      ", conf.options.build_extra
    print "Developer Mode:   ", conf.options.developer

def build(bld):
    galapix_sources = [
        "src/galapix/galapix.cpp",
        "src/galapix/viewer.cpp",
        "src/galapix/thumbnail_generator.cpp",
        "src/galapix/viewer_command.cpp"
    ]

    libgalapix_sdl_sources = [
        "src/sdl/sdl_window.cpp",
        "src/sdl/sdl_viewer.cpp"
    ]

    libgalapix_gtk_sources = [
        "src/gtk/gtk_viewer.cpp",
        "src/gtk/gtk_viewer_widget.cpp"
    ]

    optional_sources = []
    if bld.env['HAVE_SPNAV']:
        optional_sources += ["src/spnav/space_navigator.cpp"]

    libgalapix_sources = \
        glob("src/archive/*.cpp") + \
        glob("src/database/*.cpp") + \
        glob("src/database/entries/*.cpp") + \
        glob("src/database/tables/*.cpp") + \
        glob("src/display/*.cpp") + \
        glob("src/galapix/*.cpp") + \
        glob("src/galapix/layouter/*.cpp") + \
        glob("src/generator/*.cpp") + \
        glob("src/job/*.cpp") + \
        glob("src/jobs/*.cpp") + \
        glob("src/lisp/*.cpp") + \
        glob("src/math/*.cpp") + \
        glob("src/network/*.cpp") + \
        glob("src/plugins/*.cpp") + \
        glob("src/resource/*.cpp") + \
        glob("src/sqlite/*.cpp") + \
        glob("src/tools/*.cpp") + \
        glob("src/util/*.cpp")

    libgalapix_sources = [p for p in libgalapix_sources if p not in galapix_sources]

    # build logmich
    bld.stlib(target="logmich",
              source=["external/logmich/src/log.cpp",
                      "external/logmich/src/logger.cpp"],
              includes="external/logmich/include/",
              export_includes="external/logmich/include/")

    # build galapix libraries
    bld.stlib(target="galapix_sdl",
              source=libgalapix_sdl_sources,
              includes=["src/"],
              use=["WARNINGS", "logmich", "glm",
                   "SDL2", "LIBEXIF", "MAGICKXX"])

    bld.stlib(target="galapix",
              source=libgalapix_sources,
              includes=["src/"],
              use=["WARNINGS", "logmich", "glm",
                   "SDL2", "LIBEXIF", "MAGICKXX"])

    bld.program(target="galapix.sdl",
                source=galapix_sources + optional_sources,
                defines=["GALAPIX_SDL"],
                includes=["src/"],
                use=["WARNINGS", "galapix", "galapix_sdl", "logmich", "glm", "pthread", "SPNAV", "BOOST_FILESYSTEM",
                     "MAGICKXX", "SDL2", "LIBPNG", "LIBEXIF", "JPEG", "LIBCURL", "MHASH", "SQLITE3", "OPENGL"])

    if bld.options.build_galapix_gtk:
        bld.stlib(target="galapix_gtk",
                  source=libgalapix_gtk_sources,
                  includes=["src/"],
                  use=["WARNINGS", "logmich", "glm",
                       "GTKMM", "GLADEMM", "GTKGLEXTMM",
                       "SDL2", "LIBEXIF", "MAGICKXX"])

        bld.program(target="galapix.gtk",
                    source=galapix_sources + "galapix_gtk" + optional_sources,
                    defines=["GALAPIX_GTK"],
                    includes=["src/"],
                    use=["WARNINGS", "galapix", "galapix_gtk", "logmich", "glm", "pthread", "SPNAV", "BOOST_FILESYSTEM",
                         "GTKMM", "GLADEMM", "GTKGLEXTMM",
                         "MAGICKXX", "LIBPNG", "LIBEXIF", "JPEG", "LIBCURL", "MHASH", "SQLITE3", "OPENGL"])

    if bld.options.build_extra:
        for filename in glob("extra/*.cpp"):
            bld.program(target=filename[:-4],
                        source=filename,
                        includes=["src/"],
                        use=["WARNINGS", "galapix_sdl", "galapix", "logmich", "glm", "pthread", "SPNAV", "BOOST_FILESYSTEM",
                             "MAGICKXX", "LIBPNG", "LIBEXIF", "JPEG", "LIBCURL", "MHASH", "SQLITE3", "OPENGL"])
        bld.program(target="extra/imagescaler/imagescaler",
                    source="extra/imagescaler/imagescaler.cpp",
                    includes=["src/"],
                    use=["WARNINGS", "galapix", "logmich", "glm", "pthread", "SPNAV", "BOOST_FILESYSTEM",
                         "MAGICKXX", "LIBPNG", "LIBEXIF", "JPEG", "LIBCURL", "MHASH", "SQLITE3", "OPENGL"])

    if bld.options.build_tests:
        # build gtest
        bld.stlib(target="gtest",
                  source=["external/gtest-1.7.0/src/gtest-all.cc"],
                  includes=["external/gtest-1.7.0/include/",
                            "external/gtest-1.7.0/"],
                  export_includes=["external/gtest-1.7.0/include/"])
        bld.stlib(target="gtest_main",
                  source=["external/gtest-1.7.0/src/gtest_main.cc"],
                  includes=["external/gtest-1.7.0/include/",
                            "external/gtest-1.7.0/"],
                  export_includes=["external/gtest-1.7.0/include/"])

        # build automatic tests
        bld.program(target="test_galapix",
                    source=glob("test/*_test.cpp"),
                    includes=["src/"],
                    use=["gtest", "gtest_main",
                         "WARNINGS", "galapix", "galapix_sdl", "logmich", "glm", "pthread", "SPNAV", "BOOST_FILESYSTEM",
                         "MAGICKXX", "LIBPNG", "LIBEXIF", "JPEG", "LIBCURL", "MHASH", "SQLITE3", "OPENGL"])

        # build interactive tests
        for filename in glob("uitest/*_test.cpp"):
            bld.program(target=filename[:-4],
                        source=[filename],
                        includes=["src/"],
                        use=["WARNINGS", "galapix", "galapix_sdl", "logmich", "glm", "pthread", "SPNAV", "BOOST_FILESYSTEM",
                             "SDL2",
                             "MAGICKXX", "LIBPNG", "LIBEXIF", "JPEG", "LIBCURL", "MHASH", "SQLITE3", "OPENGL"])

# from waflib.Build import BuildContext, CleanContext, InstallContext, UninstallContext
# for x in ['debug', 'release', 'profile']:
#     for y in (BuildContext, CleanContext, InstallContext, UninstallContext):
#         name = y.__name__.replace('Context','').lower()
#         class tmp(y):
#             cmd = name + '_' + x
#             variant = x

# EOF #
