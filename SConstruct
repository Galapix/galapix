# -*- python -*-

env = Environment(CCFLAGS=["-Wall", "-Werror", "-O0"], LIBS=['SDL_image'])
env.ParseConfig("sdl-config --libs --cflags")

env.Program('griv', ['griv.cpp', 'loader.cpp'])

# EOF #
