# -*- mode: python -*-

compile_galapix_tests = True
build_mode = 'development'

preset_cxx = "g++-4.4"

preset_cxxflags = {
    'release':     [ "-O3", "-s"  ],
    'profile':     [ "-O2", "-g3", "-pg" ],
    'debug':       [ "-O0", "-g3" ],
    'development': [ "-O0", "-g3",
                     "-ansi",
                     "-pedantic",
                     "-Wall",
                     "-Wextra",
                     "-Wnon-virtual-dtor",
                     # "-Weffc++",
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
    }

preset_linkflags = {
    'release': [],
    'profile': [ "-pg" ],
    'debug':   [],
    'development': []
    }

Import("compile_galapix_gtk",
       "compile_galapix_sdl",
       "compile_spacenav")

libgalapix_util_sources = Glob("src/util/*.cpp") + \
                          Glob("src/plugins/*.cpp") + \
                          Glob("src/lisp/*.cpp") + \
                          Glob("src/math/*.cpp")                                               

libgalapix_sources = Glob("src/database/*.cpp") + \
                     Glob("src/display/*.cpp") + \
                     Glob("src/galapix/*.cpp") + \
                     Glob("src/job/*.cpp") + \
                     Glob("src/jobs/*.cpp") + \
                     Glob("src/sqlite/*.cpp") + \
                     Glob("src/tools/*.cpp")

galapix_sources = [
    'src/display/framebuffer.cpp',
    'src/galapix/galapix.cpp',
    'src/galapix/viewer.cpp',
    ]

spacenav_sources = ['src/spnav/space_navigator.cpp']

sdl_sources = ['src/sdl/sdl_framebuffer.cpp',
               'src/sdl/sdl_viewer.cpp']

gtk_sources =['src/gtk/gtk_viewer.cpp',
              'src/gtk/gtk_viewer_widget.cpp']

optional_sources = []
optional_defines = []
optional_libs    = []

if compile_spacenav:
    optional_sources += spacenav_sources
    optional_defines += [('HAVE_SPACE_NAVIGATOR', 1)]
    optional_libs    += ['spnav']

BuildDir('build', 'src')

libgalapix_env = Environment(CXX=preset_cxx,
                             CXXFLAGS=preset_cxxflags[build_mode],
                             LINKFLAGS=preset_linkflags[build_mode],
                             CPPPATH=['src'],
                             CPPDEFINES = optional_defines,
                             LIBS = ['GL', 'GLEW', 'sqlite3', 'jpeg', 'boost_thread-mt'] + optional_libs)
libgalapix_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
libgalapix_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
libgalapix_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
libgalapix_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')

libgalapix_util = libgalapix_env.StaticLibrary('galapix_util', libgalapix_util_sources)
libgalapix = libgalapix_env.StaticLibrary('galapix.sdl', 
                                          libgalapix_sources + optional_sources)

if compile_galapix_tests:
    libgalapix_test_env = libgalapix_env.Clone()
    libgalapix_test_env.Append(LIBS=libgalapix_util)
    libgalapix_test_env.Program("test/exec_test", ["test/exec_test.cpp"])
    libgalapix_test_env.Program("test/url_test",  ["test/url_test.cpp"])
    libgalapix_test_env.Program("test/pnm_test",  ["test/pnm_test.cpp"])
    libgalapix_test_env.Program("test/jpeg_size_test", ["test/jpeg_size_test.cpp"])
    libgalapix_test_env.Program("test/curl_test", ["test/curl_test.cpp"])

if compile_galapix_sdl:
    sdl_env = Environment(CXX=preset_cxx,
                          CXXFLAGS=preset_cxxflags[build_mode],
                          LINKFLAGS=preset_linkflags[build_mode],
                          CPPPATH=['src'],
                          CPPDEFINES = ['GALAPIX_SDL'] + optional_defines,
                          LIBS = [libgalapix, libgalapix_util,
                                  'GL', 'GLEW', 'sqlite3', 'jpeg', 'boost_thread-mt'] + optional_libs,
                          OBJPREFIX="sdl.")
    sdl_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
    sdl_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
    sdl_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
    sdl_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
    sdl_env.Program('galapix.sdl', 
                    galapix_sources + sdl_sources + optional_sources)

if compile_galapix_gtk:
    gtk_env = Environment(CXX=preset_cxx,
                          CXXFLAGS=preset_cxxflags[build_mode],
                          LINKFLAGS=preset_linkflags[build_mode],
                          CPPPATH=['src'],
                          CPPDEFINES = ['GALAPIX_GTK'] + optional_defines,
                          LIBS = [libgalapix, libgalapix_util,
                                  'GL', 'GLEW', 'sqlite3', 'jpeg', 'boost_thread-mt'] + optional_libs,
                          OBJPREFIX="gtk.")
    gtk_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
    gtk_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
    gtk_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
    gtk_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
    gtk_env.ParseConfig('pkg-config --cflags --libs gtkmm-2.4 libglademm-2.4 gtkglextmm-1.2 | sed "s/-I/-isystem/g"')
    gtk_env.Program('galapix.gtk', 
                    galapix_sources + gtk_sources + optional_sources)

# EOF #
