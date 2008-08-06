# Zoomable Image Viewer
griv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O2", "-g"], 
                       LIBS=['freeimage', 'jpeg', 'mhash', 'GL', 'GLU'])
griv_env.ParseConfig("sdl-config --libs --cflags")
griv_env.ParseConfig("pkg-config sqlite3 --libs --cflags")
griv_env.Program('griv', [
        'math.cpp',
        'math/size.cpp',
        'math/rect.cpp',
        'math/vector2i.cpp',
        'math/vector2f.cpp',
        'software_surface.cpp',
        'url.cpp',
        'sqlite.cpp',
        'filesystem.cpp',
        'file_database.cpp',
        'tile_database.cpp',
        'griv.cpp'])

# EOF #
