# -*- mode: python -*-

# Configure things here
# ------------------------
compile_galapix_sdl = True
compile_galapix_gtk = True
compile_spacenav    = True
# ------------------------

Export("compile_galapix_gtk",
       "compile_galapix_sdl",
       "compile_spacenav")

VariantDir('build', 'src', duplicate=0)
SConscript('build/SConscript')

# EOF #
