# -*- python -*-

env = Environment(CCFLAGS=["-Wall", "-Werror", "-O0", "-g"], LIBS=['SDL_image'])
env.ParseConfig("sdl-config --libs --cflags")

env.Program('griv', ['griv.cpp', 'loader.cpp', 'image.cpp', 'filesystem.cpp'])

# EOF #
