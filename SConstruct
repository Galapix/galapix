# -*- python -*-

libgriv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O0", "-g"])
libgriv = libgriv_env.StaticLibrary('griv', ['filesystem.cpp', 'md5.cpp'])

griv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O0", "-g"], LIBS=[libgriv, 'SDL_image', 'mhash', 'GL', 'GLU'])
griv_env.ParseConfig("sdl-config --libs --cflags")

griv_env.Program('griv', ['griv.cpp',
                          'loader.cpp',
                          'image.cpp',
                          'display.cpp', 
                          'texture.cpp', 
                          'workspace.cpp'])

thumbget_env = Environment(CCFLAGS=['-Wall', '-Werror', '-O0', '-g'], LIBS=['epeg'])
thumbget_env.Program('thumbget', ['thumbget.cpp'])

thumbgen_env = Environment(CCFLAGS=['-Wall', '-Werror', '-O0', '-g'], LIBS=[libgriv, 'epeg', 'mhash'])
thumbgen_env.Program('thumbgen', ['thumbgen.cpp'])

# EOF #
