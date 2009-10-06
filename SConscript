# -*- mode: python -*-

preset_cxx = "g++-4.4"

preset_cxxflags = {
    'release':     [ "-O3", "-s"  ],
    'profile':     [ "-O2", "-g3", "-pg" ],
    'debug':       [ "-O0", "-g3" ],
    'development': [ "-O2", "-g3",
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

Import("compile_galapix_gtk",
       "compile_galapix_sdl",
       "compile_spacenav")

libgalapix_sources = [
    # 'src/util/md5.cpp',
    'src/database/database.cpp',
    'src/database/file_database.cpp',
    'src/database/file_entry.cpp',
    'src/database/tile_database.cpp',
    'src/display/surface.cpp',
    'src/display/texture.cpp',
    'src/galapix/database_thread.cpp',
    'src/galapix/image.cpp',
    'src/galapix/selection.cpp',
    'src/galapix/viewer_state.cpp',
    'src/galapix/workspace.cpp',
    'src/job/job_handle.cpp',
    'src/job/job_manager.cpp',
    'src/job/job_worker_thread.cpp',
    'src/job/thread.cpp',
    'src/jobs/test_job.cpp',
    'src/jobs/tile_generation_job.cpp',
    'src/lisp/getters.cpp',
    'src/lisp/lexer.cpp',
    'src/lisp/lisp.cpp',
    'src/lisp/parser.cpp',
    'src/math/math.cpp',
    'src/math/rect.cpp',
    'src/math/size.cpp',
    'src/math/vector2f.cpp',
    'src/math/vector2i.cpp',
    'src/math/vector3f.cpp',
    'src/plugins/curl.cpp',
    'src/plugins/imagemagick.cpp',
    'src/plugins/jpeg.cpp',
    'src/plugins/jpeg_memory_dest.cpp',
    'src/plugins/jpeg_memory_src.cpp',
    'src/plugins/kra.cpp',
    'src/plugins/png.cpp',
    'src/plugins/pnm.cpp',
    'src/plugins/rar.cpp',
    'src/plugins/rsvg.cpp',
    'src/plugins/tar.cpp',
    'src/plugins/ufraw.cpp',
    'src/plugins/xcf.cpp',
    'src/plugins/zip.cpp',
    'src/sqlite/connection.cpp',
    'src/sqlite/error.cpp',
    'src/sqlite/reader.cpp',
    'src/sqlite/statement.cpp',
    'src/tools/grid_tool.cpp',
    'src/tools/move_tool.cpp',
    'src/tools/pan_tool.cpp',
    'src/tools/resize_tool.cpp',
    'src/tools/rotate_tool.cpp',
    'src/tools/view_rotate_tool.cpp',
    'src/tools/zoom_rect_tool.cpp',
    'src/tools/zoom_tool.cpp',
    'src/util/blob.cpp',
    'src/util/exec.cpp',
    'src/util/file_reader.cpp',
    'src/util/filesystem.cpp',
    'src/util/sexpr_file_reader.cpp',
    'src/util/sexpr_file_writer.cpp',
    'src/util/software_surface.cpp',
    'src/util/url.cpp',
    ]

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

# build the base library
libgalapix_env = Environment(CXX=preset_cxx,
                             CXXFLAGS=preset_cxxflags['development'],
                             CPPPATH=['src'],
                             CPPDEFINES = optional_defines,
                             LIBS = ['GL', 'GLEW', 'sqlite3', 'jpeg', 'boost_thread-mt'] + optional_libs)
libgalapix_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
libgalapix_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
libgalapix_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
libgalapix_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
libgalapix = libgalapix_env.StaticLibrary('galapix.sdl', 
                                          libgalapix_sources + optional_sources)

if compile_galapix_sdl:
    sdl_env = Environment(CXX=preset_cxx,
                          CXXFLAGS=preset_cxxflags['development'],
                          CPPPATH=['src'],
                          CPPDEFINES = ['GALAPIX_SDL'] + optional_defines,
                          LIBS = [libgalapix, 'GL', 'GLEW', 'sqlite3', 'jpeg', 'boost_thread-mt'] + optional_libs,
                          OBJPREFIX="sdl.")
    sdl_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
    sdl_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
    sdl_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
    sdl_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
    sdl_env.Program('galapix.sdl', 
                    galapix_sources + sdl_sources + optional_sources)

if compile_galapix_gtk:
    gtk_env = Environment(CXX=preset_cxx,
                          CXXFLAGS=preset_cxxflags['development'],
                          CPPPATH=['src'],
                          CPPDEFINES = ['GALAPIX_GTK'] + optional_defines,
                          LIBS = [libgalapix, 'GL', 'GLEW', 'sqlite3', 'jpeg', 'boost_thread-mt'] + optional_libs,
                          OBJPREFIX="gtk.")
    gtk_env.ParseConfig('pkg-config libpng --libs --cflags | sed "s/-I/-isystem/g"')
    gtk_env.ParseConfig('sdl-config --cflags --libs | sed "s/-I/-isystem/g"')
    gtk_env.ParseConfig('Magick++-config --libs --cppflags | sed "s/-I/-isystem/g"')
    gtk_env.ParseConfig('pkg-config --cflags --libs libcurl | sed "s/-I/-isystem/g"')
    gtk_env.ParseConfig('pkg-config --cflags --libs gtkmm-2.4 libglademm-2.4 gtkglextmm-1.2 | sed "s/-I/-isystem/g"')
    gtk_env.Program('galapix.gtk', 
                    galapix_sources + gtk_sources + optional_sources)

# EOF #
