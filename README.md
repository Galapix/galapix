[![Build Status](https://travis-ci.org/Galapix/galapix.svg?branch=develop)](https://travis-ci.org/Galapix/galapix)
[![Coverity Status](https://scan.coverity.com/projects/5576/badge.svg)](https://scan.coverity.com/projects/5576)

galapix - A Zoomable Image Viewer zoomview
==========================================

galapix is an image viewer that allows you to directly zoom into large
collection of tens of thousand of images from tiny thumbnails down to
the images original size fluently with very short load times and no
loss of interactivity. It accomplishes this by keeping a tile cache
which stores not only thumbnails for one size, but thumbnails of many
different sizes. This thumbnail cache requires at maximum 1.5x times
as much storage as the images themselves, but as tiles are generated
on demand often far less.

galapix can also be used to view gigapixel sized images. The Zoomify
ImageProperties.xml file format is support.

galapix supports a wide varity of image formats including JPEG, PNG,
XCF (Gimp), KRA (Krita) and all stuff supported by ImageMagick.


Portability
-----------

galapix is written with GNU/Linux in mind, calls to external
application, which are used to handle all kinds of file formats might
not be portable to other platforms.  There are also plenty of other
little details that will make portability to non-POSIX OSs problematic
(thread and directory handling).


Features
--------

* support for large image collections of tens of thousands of images

* support for extremely large images (gigapixel range)

* support for a wide varity of image formats (png, jpeg, xcf, kra,
  svg and everything supported by ImageMagick)

* support for a wide varity of archive formats (rar, zip, tar)

* very fast once the cache is generated

* flexible layouting functions

* abiltiy to move images around and resize them on the workspace

* very responsive, loading happens completly in the background and
  never blocks interactivity

* workspace layout can be saved and loaded

* Space Navigator is fully supported


Required Libraries and Tools:
-----------------------------

* [gtkmm](http://www.gtkmm.org/en/)
* [SDL2](https://libsdl.org/)
* [OpenGL](http://www.opengl.org/)
* [SQlite3](http://www.sqlite.org/)
* [libjpeg](http://www.ijg.org/)
* [libpng](http://www.libpng.org/pub/png/)
* [GLEW](http://glew.sourceforge.net/)
* [Magick++](http://www.imagemagick.org/Magick%2B%2B/)
* [librsvg](http://librsvg.sourceforge.net/)
* [libexif](http://libexif.sourceforge.net/)

On Ubuntu 15.04 one can install all required libraries with this call:

    sudo apt-get install \
      libboost1.55-dev \
      libboost-filesystem1.55-dev \
      libcurl4-gnutls-dev \
      libexif-dev \
      libglademm-2.4-dev \
      libglew1.5-dev \
      libgtkglextmm-x11-1.2-dev \
      libjpeg8-dev \
      libmagick++-dev \
      libpng12-dev \
      libsdl2-dev \
      libspnav-dev \
      libsqlite3-dev

The following packages are optional and used to handle some more
exotic image formats:

    sudo apt-get install \
      librsvg2-bin \
      ufraw \
      krita \
      xcftools

OpenGL is required as well, but not listed here, since installation
might differ depending on the brand of graphics card. For other
distributions library names might be slighly different, but all the
required libraries should be part of most distributions.


Compilation:
------------

To compile galapix:

    ./waf configure
    ./waf

To get a list of available options use:

    ./waf --help


Running:
--------

Galapix comes in two versions, one is `galapix.sdl`, a GUI-less SDL
based version and `galapix.gtk` a GUI version. The Gtk version is at
this point very incomplete and hasn't been tested much, but it might
be easier to use then figuring out the keybinding used by the SDL
version.

Galapix comes with a number of sub commands, allowing the user to either
view images or pre-generate thumbnails. The most basic command is:

    build/galapix.gtk view [FILES]...

Which displays the given files. To pregenerate only the small
thumbnails you use:

    build/galapix.gtk thumbgen [FILES]...

To generate not only the small thumbnails, but all tiles you use:

    build/galapix.gtk prepare [FILES]...

The other available commands are mostly for debugging and have little
actualy use.

Via the `-d` flag you can specifiy a database, so that you can have
different databases for different files. If you give an empty string
`-d ""`, no database file will be created, the database will resist in
memory only.

As a special gimmick you can run:

    build/galapix.gtk view buildin://mandelbrot

To get a classic Mandelbrot set.


Keyboard Commands for Galapix SDL
---------------------------------

Key       | Function
----------|------------------------------
F11       | toggle fullscreen
t         | toggle trackball mode (mouse is grabbed and cursor hidden)
1         | regular image layout
2         | tight image layout
3         | random image layout
4         | relayout images in a non-overlapping form
5         | layout images in a tight fashion
g         | toggle grid
h         | reset zoom and scroll
m         | switch to move/resize tools
p         | switch pan/zoom tools
b         | toggle background color
i         | only display selected images, remove the rest
n         | random shuffle images
s         | sort images alphabetically
shift+s   | sort images alphabetically in reverse order
page up   | increase gamma
page down | increase gamma
space     | print the currently visible images to stdout
delete    | remove selected images from workspace
home      | zoom in
end       | zoom out
shift     | rotate the view by moving the mouse while shift is hold down
left      | rotate the view 90
right     | rotate the view -90
up        | reset view rotation to 0 degree
escape    | exit galapix


SpaceNavigator Support
----------------------

galapix supports the Space Navigator via the Open Source spacenavd daemon:

  * [http://spacenav.sourceforge.net](http://spacenav.sourceforge.net)

The 3Dconnexion's official 3dxsrv daemon is not supported, neither is
directly reading from `/dev/input/eventX`.


Issues
------

Galapix is very picky about I/O, so if you have updatedb running in
the background or other indexing software, it can get very slow.

Incremental loading is not fully optimized, so you see more artifacts
then you should.

Running multiple instances of galapix at once can lead to:

    terminate called after throwing an instance of 'SQLiteError'
      what():  SQLiteStatement::~SQLiteStatement: column filename is not unique
    Aborted


Development
-----------

* `external/` contains several libraries included here for convenience
and to avoid dependency boat.

* `test/` contains automatic tests based on gtest, they can be run with `build/test_galapix`

* `uitest/` contains interactive tests that have to be manually
executed. Most of them accept command line parameter and provide a
little help text when run.
