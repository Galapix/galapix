# -*- python -*-

griv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O0", "-g"], LIBS=['SDL_image', 'mhash', 'GL', 'GLU'])
griv_env.ParseConfig("sdl-config --libs --cflags")

griv_env.Program('griv', ['griv.cpp',
                          'loader.cpp',
                          'image.cpp',
                          'filesystem.cpp',
                          'display.cpp', 
                          'texture.cpp', 
                          'workspace.cpp'])

thumbget_env = Environment(CCFLAGS=['-Wall', '-Werror', '-O0', '-g'], LIBS=['epeg'])
thumbget_env.Program('thumbget', ['thumbget.cpp'])

# EOF #
