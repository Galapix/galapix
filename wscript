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

def options(opt):
    opt.load('compiler_cxx')
    opt.add_option('--gtk', action='store', default=False, help='Build galapix.gtk')
    opt.add_option('--sdl', action='store', default=True, help='Build galapix.sdl')
    opt.load('compiler_cxx boost')

def configure(conf):
    conf.load('g++')
    conf.load('compiler_cxx boost')
    conf.check_boost(lib='system filesystem')
    conf.check_cfg(atleast_pkgconfig_version='0.0.0')
    conf.check_cfg(package='sdl2', args=['--cflags', '--libs'])
    conf.check_cfg(package='libexif', args=['--cflags', '--libs'])
    conf.check_cfg(package='sqlite3', args=['--cflags', '--libs'])
    conf.check_cfg(package='mhash', args=['--cflags', '--libs'])
    conf.check_cfg(package='libcurl', args=['--cflags', '--libs'])
    conf.check_cfg(package='Magick++', args=['--cflags', '--libs'], uselib_store="MAGICKXX")
    conf.check_cfg(package='libpng', args=['--cflags', '--libs'])
    conf.check_cfg(package='glew', args=['--cflags', '--libs'])
    conf.check_cfg(package='gl', args=['--cflags', '--libs'])
    conf.check_cxx(lib='jpeg')
                
    # conf.link_add_flags("-pthread")

    #if conf.CheckLibWithHeader("spnav", "spnav.h", "c++"):
    #    self.optional_sources += ["src/spnav/space_navigator.cpp"]
    #    self.optional_defines += [("HAVE_SPACE_NAVIGATOR", 1)]
    #    self.optional_libs    += ["spnav"]

    print(conf.env)

def build(bld):
    glob = bld.path.ant_glob

    galapix_sources = [
        "src/display/framebuffer.cpp",
        "src/galapix/galapix.cpp",
        "src/galapix/viewer.cpp",
        "src/galapix/thumbnail_generator.cpp",
        "src/galapix/viewer_command.cpp"
    ]

    sdl_sources = [
        "src/sdl/sdl_window.cpp",
        "src/sdl/sdl_viewer.cpp"
    ]

    optional_sources = []

    libgalapix_util_sources = \
        glob("src/archive/*.cpp") + \
        glob("src/network/*.cpp") + \
        glob("src/generator/*.cpp") + \
        glob("src/util/*.cpp") + \
        glob("src/plugins/*.cpp") + \
        glob("src/lisp/*.cpp") + \
        glob("src/jobs/*.cpp") + \
        glob("src/resource/*.cpp") + \
        glob("src/math/*.cpp")

    libgalapix_sources = \
        glob("src/database/*.cpp") + \
        glob("src/database/entries/*.cpp") + \
        glob("src/database/tables/*.cpp") + \
        glob("src/display/*.cpp") + \
        glob("src/galapix/*.cpp") + \
        glob("src/galapix/layouter/*.cpp") + \
        glob("src/job/*.cpp") + \
        glob("src/sqlite/*.cpp") + \
        glob("src/tools/*.cpp")
        # optional_sources

    liblogmich_sources = [
        "external/logmich/src/log.cpp",
        "external/logmich/src/logger.cpp"
    ]
        
    extra_includes = [
        "src/",
        "external/logmich/include/",
        "external/glm-0.9.6.1/"
    ]

    extra_cxxflags = ["-std=c++1y", "-O3", "-s", "-pthread"]

    bld.stlib(target="logmich",
              source=liblogmich_sources,
              cxxflags=extra_cxxflags,
              includes=extra_includes)
    
    bld.stlib(target="galapix_util",
              source=libgalapix_util_sources,
              cxxflags=extra_cxxflags,
              includes=extra_includes,
              # uselibs are uppercase '-' is replaced with '_'
              use=["SDL2", "LIBEXIF", "MAGICKXX"])

    bld.stlib(target="galapix",
              source=libgalapix_sources,
              cxxflags=extra_cxxflags,
              includes=extra_includes,
              # uselibs are uppercase '-' is replaced with '_'
              use=["SDL2", "LIBEXIF", "MAGICKXX"])

    print(bld.env)
    
    bld.program(target="galapix.sdl",
                source=galapix_sources + optional_sources,
                cxxflags=extra_cxxflags,
                linkflags=["-pthread"],
                includes=extra_includes,
                use=["galapix_sdl", "galapix", "galapix_util", "logmich",
                     "MAGICKXX", "SDL2", "LIBPNG", "LIBEXIF", "JPEG", "LIBCURL", "MHASH", "SQLITE3", "GL", "GLEW", "BOOST"])

    print(bld.env)

# EOF #
