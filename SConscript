# -*- mode: python -*-

import glob
import os

# CacheDir('cache')

preset_cxxflags = {
    'release':     [ "-std=c++11", "-O3", "-s"  ],
    'profile':     [ "-std=c++11", "-O2", "-g3", "-pg" ],
    'debug':       [ "-std=c++11", "-O0", "-g3" ],
    'development': [ "-std=c++11", "-O0", "-g3",
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
    }

preset_linkflags = {
    'release': [],
    'profile': [ "-pg" ],
    'debug':   [],
    'development': []
    }

class Project:
    def __init__(self):
        self.optional_sources = []
        self.optional_defines = []
        self.optional_libs    = []

        self.galapix_sources = [
            'src/display/framebuffer.cpp',
            'src/galapix/galapix.cpp',
            'src/galapix/viewer.cpp',
        ]

        self.sdl_sources = [
            'src/sdl/sdl_window.cpp',
            'src/sdl/sdl_viewer.cpp'
        ]
        
    def configure(self):       
        if 'BUILD' in self.env:
            print("Build type: %s" % self.env['BUILD'])
            self.env.Append(CXXFLAGS  = preset_cxxflags[self.env['BUILD']],
                            LINKFLAGS = preset_linkflags[self.env['BUILD']])
        else:
            print("Build type: release")
            self.env.Append(CXXFLAGS  = preset_cxxflags['release'],
                            LINKFLAGS = preset_linkflags['release'])

        conf = Configure(self.env)

        if self.env['CXX']:
            print("Using C++ compiler...", self.env['CXX'])
        else:
            print("Error: C++ compiler missing")
            Exit(1)

        if conf.CheckLibWithHeader("spnav", "spnav.h", "c++"):
            self.optional_sources += ['src/spnav/space_navigator.cpp']
            self.optional_defines += [('HAVE_SPACE_NAVIGATOR', 1)]
            self.optional_libs    += ['spnav']

        # if not conf.CheckLibWithHeader("boost_thread", "boost/thread.hpp", "c++", autoadd=0):
        #     print "Error: boost_thread is missing"
        #     Exit(1)
            
        if not conf.CheckHeader("boost/signals2/signal.hpp", "<>", "c++"):
            print("Error: boost_signals2 is missing")
            Exit(1)

        if not conf.CheckLibWithHeader("exif", "libexif/exif-data.h", "c++", autoadd=0):
            print("Error: libexif is missing")
            Exit(1)

        if not conf.CheckLibWithHeader("sqlite3", "sqlite3.h", "c++", autoadd=0):
            print("Error: sqlite3 is missing")
            Exit(1)

        if not conf.CheckLib("jpeg", autoadd=0):
            print("Error: libjpeg is missing")
            Exit(1)

        if not conf.CheckLibWithHeader("GL", "GL/gl.h", "c++", autoadd=0):
            print("Error: libGL is missing")
            Exit(1)

        if not conf.CheckLibWithHeader("GLEW", "GL/glew.h", "c++", autoadd=0):
            print("Error: libGLEW is missing")
            Exit(1)

        self.env = conf.Finish()

    def build(self):
        self.env = Environment(ENV = {'PATH' : os.environ['PATH'],
                                      'HOME' : os.environ['HOME']},
                               CPPPATH=['src'])

        opts = Variables(['custom.py'], ARGUMENTS)
        opts.Add('CXX', 'C++ Compiler')
        opts.Add('CXXFLAGS', 'C++ Compiler Flags')
        opts.Add('BUILD', 'Build type: release, profile, debug, development')
        opts.Add(BoolVariable("GALAPIX_SDL", "Build galapix.sdl", True))
        opts.Add(BoolVariable("GALAPIX_GTK", "Build galapix.gtk", True))
        opts.Add(BoolVariable("BUILD_TESTS", "Build tests", False))
        opts.Add(BoolVariable('BUILD_EXTRA_APPS', "Build extra apps", True))
        Help(opts.GenerateHelpText(self.env))
        opts.Update(self.env)

        self.configure()
        
        self.build_libgalapix();

        if self.env['GALAPIX_SDL']:
            self.build_galapix_sdl()

        if self.env['GALAPIX_GTK']:
            self.build_galapix_gtk()

        if self.env['BUILD_TESTS']:
            self.build_tests()

        if self.env['BUILD_EXTRA_APPS']:
            self.build_extra_apps()

    def build_libgalapix(self):
        self.libgalapix_env = self.env.Clone()
        self.libgalapix_env.Append(CPPDEFINES = self.optional_defines,
                                   LIBS = ['GL', 'GLEW', 'sqlite3', 'jpeg', 'exif', 'boost_filesystem'] + self.optional_libs)
        self.libgalapix_env.ParseConfig('pkg-config --cflags --libs libpng  | sed "s/-I/-isystem/g"')
        self.libgalapix_env.ParseConfig('pkg-config --cflags --libs sdl2 | sed "s/-I/-isystem/g"')
        self.libgalapix_env.ParseConfig('pkg-config --cflags --libs Magick++ | sed "s/-I/-isystem/g"')
        self.libgalapix_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
        
        self.libgalapix_util = self.libgalapix_env.StaticLibrary('galapix_util',
                                                                 Glob("src/util/*.cpp") + \
                                                                 Glob("src/plugins/*.cpp") + \
                                                                 Glob("src/lisp/*.cpp") + \
                                                                 Glob("src/math/*.cpp"))
        
        self.libgalapix = self.libgalapix_env.StaticLibrary('galapix.sdl', 
                                                            Glob("src/database/*.cpp") + \
                                                            Glob("src/display/*.cpp") + \
                                                            Glob("src/galapix/*.cpp") + \
                                                            Glob("src/job/*.cpp") + \
                                                            Glob("src/jobs/*.cpp") + \
                                                            Glob("src/sqlite/*.cpp") + \
                                                            Glob("src/tools/*.cpp") + \
                                                            self.optional_sources)
    def build_galapix_sdl(self):
        sdl_env = self.env.Clone()
        sdl_env.Append(CPPDEFINES = ['GALAPIX_SDL'] + self.optional_defines,
                       LIBS = [self.libgalapix, self.libgalapix_util,
                               'GL', 'GLEW', 'sqlite3', 'jpeg', 'exif'] + self.optional_libs,
                       OBJPREFIX="sdl.")
        sdl_env.ParseConfig('pkg-config --cflags --libs libpng | sed "s/-I/-isystem/g"')
        sdl_env.ParseConfig('pkg-config --cflags --libs sdl2 | sed "s/-I/-isystem/g"')
        sdl_env.ParseConfig('pkg-config --cflags --libs Magick++ | sed "s/-I/-isystem/g"')
        sdl_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
        sdl_env.Program('galapix.sdl', 
                        self.sdl_sources + \
                        self.galapix_sources + \
                        self.optional_sources)


    def build_galapix_gtk(self):
        gtk_env = self.env.Clone()
        gtk_env.Append(CPPDEFINES = ['GALAPIX_GTK'] + self.optional_defines,
                       LIBS = [self.libgalapix, self.libgalapix_util,
                               'GL', 'GLEW', 'sqlite3', 'jpeg', 'exif'] + self.optional_libs,
                       OBJPREFIX="gtk.")
        gtk_env.ParseConfig('pkg-config --cflags --libs libpng | sed "s/-I/-isystem/g"')
        gtk_env.ParseConfig('pkg-config --cflags --libs sdl2 | sed "s/-I/-isystem/g"')
        gtk_env.ParseConfig('pkg-config --cflags --libs Magick++ | sed "s/-I/-isystem/g"')
        gtk_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
        gtk_env.ParseConfig('pkg-config --cflags --libs gtkmm-2.4 libglademm-2.4 gtkglextmm-1.2 | sed "s/-I/-isystem/g"')
        gtk_env.Program('galapix.gtk', 
                        ['src/gtk/gtk_viewer.cpp',
                         'src/gtk/gtk_viewer_widget.cpp'] + \
                        self.galapix_sources + \
                        self.optional_sources)

    def build_tests(self):
        libgalapix_test_env = self.libgalapix_env.Clone()
        libgalapix_test_env.Prepend(LIBS=self.libgalapix_util)
        for filename in Glob("test/*_test.cpp", strings=True):
            libgalapix_test_env.Program(filename[:-4], filename)

    def build_extra_apps(self):
        libgalapix_extra_apps_env = self.libgalapix_env.Clone()
        libgalapix_extra_apps_env.Prepend(LIBS=self.libgalapix_util)
        for filename in Glob("extra/*.cpp", strings=True):
            libgalapix_extra_apps_env.Program(filename[:-4], filename)

        libgalapix_extra_apps_env.Program("extra/imagescaler/imagescaler", Glob("extra/imagescaler/imagescaler.cpp"))
                    
project = Project()
project.build()

# EOF #
