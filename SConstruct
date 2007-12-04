# -*- python -*-

# Thumbnail generation library
libepeg_env = Environment(CCFLAGS=["-Wall", "-O2", "-g"])
libepeg = libepeg_env.StaticLibrary('epeg', ['epeg/epeg_main.c'])

# Shared code between Griv and Thumbgen
libgriv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O2", "-g"])
libgriv_env.ParseConfig("sdl-config --libs --cflags")
libgriv = libgriv_env.StaticLibrary('griv', ['filesystem.cpp', 
                                             'command_line.cpp',
                                             'jpeg.cpp',
                                             'file_entry_cache.cpp',
                                             'md5.cpp'])

# Zoomable Image Viewer
griv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O2", "-g"], 
                       LIBS=[libgriv, libepeg, 'SDL_image', 'jpeg', 'mhash', 'GL', 'GLU'])
griv_env.ParseConfig("sdl-config --libs --cflags")
griv_env.Program('griv', ['griv.cpp',
                          'loader.cpp',
                          'image.cpp',
                          'surface.cpp',
                          'large_surface.cpp',
                          'software_surface.cpp',
                          'thumbnail_store.cpp',
                          'thumbnail_manager.cpp',
                          'display.cpp', 
                          'texture.cpp', 
                          'workspace.cpp'])

# Offline Thumbnail generator
thumbgen_env = Environment(CCFLAGS=['-Wall', '-Werror', '-O2', '-g'], 
                           LIBS=[libgriv, libepeg, 'jpeg', 'mhash'])
thumbgen_env.Program('thumbgen', ['thumbgen.cpp'])

# packer_env = Environment(CCFLAGS=['-Wall', '-Werror', '-O2', '-g'], 
#                            LIBS=[libgriv, libepeg, 'jpeg', 'mhash'])
# packer_env.Program('packer', ['packer.cpp'])

# EOF #
