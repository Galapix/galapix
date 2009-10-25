# -*- mode: python -*-

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

    def configure(self):       
        if 'BUILD' in self.env:
            print "Build type: %s" % self.env['BUILD']
            self.env.Append(CXXFLAGS  = preset_cxxflags[self.env['BUILD']],
                            LINKFLAGS = preset_linkflags[self.env['BUILD']])
        else:
            print "Build type: release"
            self.env.Append(CXXFLAGS  = preset_cxxflags['release'],
                            LINKFLAGS = preset_linkflags['release'])

        conf = Configure(self.env)

        if self.env['CXX']:
            print "Using C++ compiler...", self.env['CXX']
        else:
            print "Error: C++ compiler missing"
            Exit(1)

        if conf.CheckLibWithHeader("spnav", "spnav.h", "c++"):
            self.optional_sources += ['src/spnav/space_navigator.cpp']
            self.optional_defines += [('HAVE_SPACE_NAVIGATOR', 1)]
            self.optional_libs    += ['spnav']

        if not conf.CheckLibWithHeader("boost_thread-mt", "boost/thread.hpp", "c++", autoadd=0):
            print "Error: boost_thread-mt is missing"
            Exit(1)
            
        if not conf.CheckLibWithHeader("boost_signals-mt", "boost/signals.hpp", "c++", autoadd=0):
            print "Error: boost_signals-mt is missing"
            Exit(1)

        if not conf.CheckLibWithHeader("exif", "libexif/exif-data.h", "c++", autoadd=0):
            print "Error: libexif is missing"
            Exit(1)

        if not conf.CheckLibWithHeader("sqlite3", "sqlite3.h", "c++", autoadd=0):
            print "Error: sqlite3 is missing"
            Exit(1)

        if not conf.CheckLib("jpeg", autoadd=0):
            print "Error: libjpeg is missing"
            Exit(1)

        if not conf.CheckLibWithHeader("GL", "GL/gl.h", "c++", autoadd=0):
            print "Error: libGL is missing"
            Exit(1)

        if not conf.CheckLibWithHeader("GLEW", "GL/glew.h", "c++", autoadd=0):
            print "Error: libGLEW is missing"
            Exit(1)

        self.env = conf.Finish()

    def build(self):
        self.env = Environment(CPPPATH=['src'])

        opts = Variables(['custom.py'], ARGUMENTS)
        opts.Add('CXX', 'C++ Compiler')
        opts.Add('BUILD', 'Build type: release, profile, debug, development')
        opts.Add(BoolVariable("GALAPIX_SDL", "Build galapix.sdl", True))
        opts.Add(BoolVariable("GALAPIX_GTK", "Build galapix.gtk", True))
        opts.Add(BoolVariable("BUILD_TESTS", "Build tests", False))
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

    def build_libgalapix(self):
        self.libgalapix_env = self.env.Clone()
        self.libgalapix_env.Append(CPPDEFINES = self.optional_defines,
                                   LIBS = ['GL', 'GLEW', 'sqlite3', 'jpeg', 'exif', 'boost_thread-mt', 'boost_signals-mt'] + self.optional_libs)
        self.libgalapix_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
        self.libgalapix_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
        self.libgalapix_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
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
                               'GL', 'GLEW', 'sqlite3', 'jpeg', 'exif', 'boost_thread-mt', 'boost_signals-mt'] + self.optional_libs,
                       OBJPREFIX="sdl.")
        sdl_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
        sdl_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
        sdl_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
        sdl_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
        sdl_env.Program('galapix.sdl', 
                        ['src/sdl/sdl_framebuffer.cpp',
                         'src/sdl/sdl_viewer.cpp'] + \
                        self.galapix_sources + \
                        self.optional_sources)


    def build_galapix_gtk(self):
        gtk_env = self.env.Clone()
        gtk_env.Append(CPPDEFINES = ['GALAPIX_GTK'] + self.optional_defines,
                       LIBS = [self.libgalapix, self.libgalapix_util,
                               'GL', 'GLEW', 'sqlite3', 'jpeg', 'exif', 'boost_thread-mt', 'boost_signals-mt'] + self.optional_libs,
                       OBJPREFIX="gtk.")
        gtk_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
        gtk_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
        gtk_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
        gtk_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
        gtk_env.ParseConfig('pkg-config --cflags --libs gtkmm-2.4 libglademm-2.4 gtkglextmm-1.2 | sed "s/-I/-isystem/g"')
        gtk_env.Program('galapix.gtk', 
                        ['src/gtk/gtk_viewer.cpp',
                         'src/gtk/gtk_viewer_widget.cpp'] + \
                        self.galapix_sources + \
                        self.optional_sources)

    def build_tests(self):
        libgalapix_test_env = self.libgalapix_env.Clone()
        libgalapix_test_env.Append(LIBS=self.libgalapix_util)
        libgalapix_test_env.Program("test/exec_test",
                                    ["test/exec_test.cpp"])
        libgalapix_test_env.Program("test/url_test",
                                    ["test/url_test.cpp"])
        libgalapix_test_env.Program("test/pnm_test",
                                    ["test/pnm_test.cpp"])
        libgalapix_test_env.Program("test/jpeg_size_test",
                                    ["test/jpeg_size_test.cpp"])
        libgalapix_test_env.Program("test/curl_test",
                                    ["test/curl_test.cpp"])
        libgalapix_test_env.Program("test/exif_test",
                                    ["test/exif_test.cpp"])
        libgalapix_test_env.Program("test/signals_test",
                                    ["test/signals_test.cpp"])
        libgalapix_test_env.Program("test/software_surface_test",
                                    ["test/software_surface_test.cpp"])

project = Project()
project.build()

# EOF #
