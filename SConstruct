# -*- python -*-

libgriv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O2", "-g"])
libgriv = libgriv_env.StaticLibrary('griv', ['filesystem.cpp', 
                                             'command_line.cpp',
                                             'md5.cpp'])

libepeg_env = Environment(CCFLAGS=["-Wall", "-O2", "-g"])
libepeg = libepeg_env.StaticLibrary('epeg', ['epeg/epeg_main.c'])

griv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O2", "-g"], 
                       LIBS=[libgriv, libepeg, 'SDL_image', 'jpeg', 'mhash', 'GL', 'GLU'])
griv_env.ParseConfig("sdl-config --libs --cflags")
griv_env.Program('griv', ['griv.cpp',
                          'loader.cpp',
                          'image.cpp',
                          'thumbnail_store.cpp',
                          'display.cpp', 
                          'texture.cpp', 
                          'workspace.cpp'])

thumbgen_env = Environment(CCFLAGS=['-Wall', '-Werror', '-O2', '-g'], 
                           LIBS=[libgriv, libepeg, 'jpeg', 'mhash'])
thumbgen_env.Program('thumbgen', ['thumbgen.cpp'])

# EOF #
