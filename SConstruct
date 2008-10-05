galapix_sources = [
    #    'src/md5.cpp',
    'src/blob.cpp',
    'src/database_thread.cpp',
    'src/curl.cpp',
    'src/exec.cpp',
    'src/file_database.cpp',
    'src/file_entry.cpp',
    'src/filesystem.cpp',
    'src/framebuffer.cpp',
    'src/file_reader.cpp',
    'src/lisp/getters.cpp',
    'src/lisp/lexer.cpp',
    'src/lisp/lisp.cpp',
    'src/lisp/parser.cpp',
    'src/sexpr_file_reader.cpp',
    'src/sexpr_file_writer.cpp',
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

import sys, os, SCons

class _SpaceListOptionClass:
   """An option type for space-separated lists with arbitrary elements."""
   def CheckDir(self, val):
      if not os.path.isdir(val):
         raise SCons.Errors.UserError("No directory at %s" % val)

   def _convert(self, key, val, env):
      if SCons.Util.is_List(val): # prefer val if it's already a list
         return val
      elif len(val) > 0 and val[0] == '[' and val[-1] == ']':
         # or a repr of a list
         return eval(val)
      elif env: # otherwise, use whatever's in env
         val = env[key]
         if not SCons.Util.is_List(val):
            val = val.split(None)
         return val
      else: # val was substituted into a string, losing its structure
         # We'll be called again with env, hopefully that's more useful
         raise TypeError("try again with the environment")

   def _validate(self, val, env, check, converter):
      for i in converter(val, env):
         if check(i):
            return True
      return False

   def __call__(self, key, help, check=None, default=[]):
      def converter(val, env = None):
         return self._convert(key, val, env)
 
      validator = None
      if check is not None:
         validator = lambda k, v, e: self._validate(v, e, check, converter)
      return (key, help, default, validator, converter)
 
SpaceListOption = _SpaceListOptionClass()

def DefineOptions(filename, args):
    opts = Options(filename, args)
    opts.Add('CC', 'C Compiler', 'gcc')
    opts.Add('CXX', 'C++ Compiler', 'g++')
#   opts.Add('debug', 'Build with debugging options', 0)
#   opts.Add('profile', 'Build with profiling support', 0)

    opts.Add(SpaceListOption('CPPPATH',    'Additional preprocessor paths'))
    opts.Add(SpaceListOption('LIBPATH',    'Additional library paths'))
    opts.Add(SpaceListOption('CPPFLAGS',   'Additional preprocessor flags'))
    opts.Add(SpaceListOption('CPPDEFINES', 'defined constants'))
    opts.Add(SpaceListOption('LIBS',       'Additional libraries'))
    opts.Add(SpaceListOption('CCFLAGS',    'C Compiler flags'))
    opts.Add(SpaceListOption('CXXFLAGS',   'C++ Compiler flags'))
    opts.Add(SpaceListOption('LINKFLAGS',  'Linker Compiler flags'))

    opts.Add(BoolOption('with_space_navigator', 'Build with Space Navigator support', True))
    opts.Add(BoolOption('ignore_errors',        'Ignore any fatal configuration errors', False))
    opts.Add('optional_sources', 'Additional source files', [])
    return opts

def CheckMyProgram(context, prgn):
    context.Message('Checking for %s...' % prgn)
    for i in context.env['ENV']['PATH'].split(":"):
        if os.path.exists(i + "/" + prgn):
            context.Result(i + "/" + prgn)
            return True
    context.Result("failed")
    return False

def CheckSDLLib(context, sdllib):
    """
    On some platforms, SDL does this ugly redefine-main thing, that can
    interact badly with CheckLibWithHeader.
    """
    lib = "SDL_%s" % sdllib
    context.Message('Checking for %s...' % lib)
    text = """
#include "SDL.h"
#include "%s.h"
int main(int argc, char* argv[]) { return 0; }
""" % lib
    context.AppendLIBS(lib)
    if context.BuildProg(text, ".cpp"):
        context.Result("failed")
        return False
    else:
        context.Result("ok")
        return True

def parse_config(env, args):
    for arg in args:
        if arg[:2] == '-L':
            env['LIBPATH'].append(arg[2:])
        elif arg[:2] == '-l':
            env['LIBS'].append(arg[2:])
        elif arg[:2] == '-I':
            env['CPPPATH'].append(arg[2:])
        else:
            print "Error: Unhandled", arg

Alias('configure')

if ('configure' in COMMAND_LINE_TARGETS) or \
        not (os.path.exists('config.py') and os.path.exists('config.h')) and \
        not GetOption('clean'):
    opts = DefineOptions(None, ARGUMENTS)
    env = Environment(options = opts)
    Help(opts.GenerateHelpText(env))

    opts.Update(env)

    if os.environ.has_key('PATH'):
        env['ENV']['PATH'] = os.environ['PATH']
    if os.environ.has_key('HOME'):
        env['ENV']['HOME'] = os.environ['HOME']
    if os.environ.has_key('PKG_CONFIG_PATH'):
        env['ENV']['PKG_CONFIG_PATH'] = os.environ['PKG_CONFIG_PATH']

    config_h_defines = []      

    config = env.Configure(custom_tests = {
            'CheckMyProgram' : CheckMyProgram,
            'CheckSDLLib': CheckSDLLib,
            })

    fatal_error = ""
    reports = ""
    #------------------------------------------------------------------
    # -- Check for SDL
    if config.CheckMyProgram('sdl-config'):
        env.ParseConfig('sdl-config  --cflags --libs')
    else:
        fatal_error += "  * couldn't find sdl-config, SDL missing\n"

    #------------------------------------------------------------------
    # -- Check for OpenGL Libraries
    # FIXME: Add check for OpenGL libraries
    reports += "  * OpenGL support:         enabled\n"
    config_h_defines  += [('HAVE_OPENGL', 1)]
    env['LIBS']       += ['GL']

    env['LIBS']       += ['GLEW']

    
    #------------------------------------------------------------------
    # -- Check for Space Navigator
    if not env['with_space_navigator']:
        reports += "  * SpaceNavigator support: disabled\n"
    else:
        if config.CheckLibWithHeader('spnav', 'spnav.h', 'c++'):
            reports += "  * SpaceNavigator support: enabled\n"
            config_h_defines  += [('HAVE_SPACE_NAVIGATOR', 1)]
            env['LIBS']       += ['spnav']
            env['optional_sources'] += ['src/space_navigator.cpp']
        else:
            reports += "  * SpaceNavigator support: disabled, libspnav not found\n"

    #------------------------------------------------------------------
    # -- Check for libjpeg
    env['LIBS'] += ['jpeg']
# FIXME: Doesn't work, jpeg wants stdio.h to work
#     if config.CheckLibWithHeader('jpeg', 'jpeglib.h', 'c++'):
#         env['LIBS'] += ['jpeg']
#         reports += "  * libjpeg found\n"
#     else:
#         fatal_error += "  * libjpeg library not found\n"

    #------------------------------------------------------------------
    # -- Check for libpng
    env['LIBS'] += ['png']
# FIXME: Add proper check

    #------------------------------------------------------------------
    # -- Check for Magick++
    if config.CheckMyProgram('Magick++-config'):
        # env['CXXFLAGS']  += os.popen("Magick++-config --cxxflags").read().split()
        env['CPPFLAGS']  += os.popen("Magick++-config --cppflags").read().split()
        # env['LINKFLAGS'] += os.popen("Magick++-config --ldflags").read().split()
        parse_config(env, os.popen("Magick++-config --libs").read().split())
    else:
        fatal_error += "  * couldn't find Magick++-config, Magick++ missing\n"

    #------------------------------------------------------------------
    # -- Check for SQlite3
    if config.CheckLibWithHeader('sqlite3', 'sqlite3.h', 'c++'):
        env['LIBS'] += ['sqlite3']
        reports += "  * sqlite3 found\n"
    else:
        fatal_error += "  * sqlite3 library not found\n"

    #------------------------------------------------------------------
    # -- Check for cURL        
    env['CPPFLAGS'] += os.popen("pkg-config --cflags libcurl").read().split()
    env['LIBS']     += os.popen("pkg-config --libs libcurl").read().split()

    #------------------------------------------------------------------
    # -- Check for boost    
    # not implemented

    #------------------------------------------------------------------
    # -- Finish up
    env = config.Finish()
    opts.Save("config.py", env)

    print "Reports:"
    print reports

    if not fatal_error == "":
        print "Fatal Errors:"
        print fatal_error
        if not env['ignore_errors']:
            Exit(1)
        else:
            print "\nError are being ignored, the build continues"

    config_h = open('config.h', 'w')
    config_h.write('#define VERSION "0.0.4"\n')
    for (v,k) in config_h_defines:
        config_h.write('#define %s %s\n' % (v, k))
    config_h.close()
    if ('configure' in COMMAND_LINE_TARGETS):
        print "Configuration written to config.h and config.py, run:"
        print ""
        print "  scons"
        print ""
        print "To start the compile"
    else:
        print "Configuration written to config.h and config.py"
    ARGUMENTS = {}

if not ('configure' in COMMAND_LINE_TARGETS):
    opts = DefineOptions("config.py", {})
    env = Environment(options = opts)
    Help(opts.GenerateHelpText(env))

    opts.Update(env)
    Default(env.Program('galapix', galapix_sources + env['optional_sources']))
    Clean('galapix', ['config.py', 'config.h'])

# EOF #
