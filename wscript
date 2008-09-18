# -*- python -*-

# the following two variables are used by the target "waf dist"
VERSION='0.0.4'
APPNAME='galapix'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'

galapix_sources = [
    'src/blob.cpp',
    'src/database_thread.cpp',
    'src/curl.cpp',
    'src/exec.cpp',
    'src/file_database.cpp',
    'src/file_entry.cpp',
    'src/filesystem.cpp',
    'src/framebuffer.cpp',
    'src/galapix.cpp',
    'src/kra.cpp',
    'src/image.cpp',
    'src/imagemagick.cpp',
    'src/job_handle.cpp',
    'src/jpeg.cpp',
    'src/jpeg_decoder_thread.cpp',
    'src/jpeg_memory_dest.cpp',
    'src/jpeg_memory_src.cpp',
    'src/math.cpp',
    'src/math/rect.cpp',
    'src/math/size.cpp',
    'src/math/vector2f.cpp',
    'src/math/vector2i.cpp',
    'src/math/vector3f.cpp',
    'src/move_tool.cpp',
    'src/pan_tool.cpp',
    'src/png.cpp',
    'src/pnm.cpp',
    'src/resize_tool.cpp',
    'src/rotate_tool.cpp',
    'src/rar.cpp',
    'src/selection.cpp',
    'src/software_surface.cpp',
    'src/sqlite.cpp',
    'src/surface.cpp',
    'src/texture.cpp',
    'src/thread.cpp',
    'src/tile_database.cpp',
    'src/tile_generator.cpp',
    'src/tile_generator_thread.cpp',
    'src/view_rotate_tool.cpp',
    'src/url.cpp',
    'src/viewer.cpp',
    'src/viewer_state.cpp',
    'src/viewer_thread.cpp',
    'src/tar.cpp',
    'src/workspace.cpp',
    'src/xcf.cpp',
    'src/zoom_tool.cpp',
    'src/zip.cpp',
    ]

def init():
    # import Params
    # Params.g_autoconfig=1
    pass

def set_options(opt):
    opt.tool_options('compiler_cxx')

def configure(conf):
    conf.check_tool('compiler_cxx')
 
    cfgtool = conf.create_cfgtool_configurator()
    cfgtool.binary = 'sdl-config'
    cfgtool.uselib ='SDL'
    cfgtool.run()

    cfgtool = conf.create_pkgconfig_configurator()
    cfgtool.name   = 'libcurl'
    cfgtool.uselib = 'curl'
    cfgtool.run()

    # conf.check_pkg('libcurl')
    
    cfgtool = conf.create_pkgconfig_configurator()
    cfgtool.name = 'libpng'
    cfgtool.uselib ='png'
    cfgtool.run()

    cfgtool = conf.create_pkgconfig_configurator()
    cfgtool.name = 'ImageMagick++'
    cfgtool.uselib ='ImageMagick++'
    cfgtool.run()
    
    # FIXME: Hardcoded unchecked libraries
    conf.env['LIB_OpenGL']  = ['GL', 'GLEW']
    conf.env['LIB_SQLite']  = ['sqlite3']
    conf.env['LIB_SpNav']   = ['spnav']

    # 'spnav', 'spnav.h', 'c++'
    # cfgtool = conf.create_header_configurator()
    # cfgtool.name = 'spnav.h'
    # cfgtool.define = 'HAVE_SPACE_NAVIGATOR'
    # cfgtool.run()

    conf.define('VERSION', VERSION)
    conf.define('HAVE_SPACE_NAVIGATOR', 1)
    conf.write_config_header('src/config.h')

def build(bld):
    obj = bld.create_obj('cpp', 'program')
    obj.source = galapix_sources + ['src/space_navigator.cpp'] # FIXME: Make this optional
    obj.target = 'galapix'
    obj.uselib = ['SDL', 'curl', 'png', 'ImageMagick++', 'OpenGL', 'SQLite', 'SpNav']

# EOF #
