# -*- python -*-

# the following two variables are used by the target "waf dist"
VERSION='0.0.4'
APPNAME='galapix'

# these variables are mandatory ('/' are converted automatically)
srcdir = 'src'
blddir = 'build'

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
    bld.add_subdirs('src')

# EOF #
