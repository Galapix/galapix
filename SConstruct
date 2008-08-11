# Zoomable Image Viewer
griv_env = Environment(CCFLAGS=["-Wall", "-Werror", "-O2", "-g", "-Wnon-virtual-dtor"], 
                       LIBS=['freeimage', 
                             'mhash', 
                             'GL', 
                             'GLU'])
griv_env.ParseConfig("sdl-config --libs --cflags")
griv_env.ParseConfig("pkg-config sqlite3 --libs --cflags")
griv_env.Program('griv', [
        'math.cpp',
        'math/size.cpp',
        'math/rect.cpp',
        'math/vector2i.cpp',
        'math/vector2f.cpp',
        'math/vector3f.cpp',
        'file_database.cpp',
        'filesystem.cpp',
        'framebuffer.cpp',
        'griv.cpp',
        'image.cpp',
        'md5.cpp',
        'software_surface.cpp',
        'sqlite.cpp',
        'surface.cpp',
        'texture.cpp',
        'thread.cpp',
        'tile_database.cpp',
        'tile_generator.cpp',
        'database_thread.cpp',
        'viewer_thread.cpp',
        'url.cpp',
        'viewer.cpp',
        'workspace.cpp',
        ])

# EOF #
