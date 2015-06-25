#!/usr/bin/env python3
##
##  Galapix - an image viewer for large image collections
##  Copyright (C) 2012 Ingo Ruhnke <grumbel@gmail.com>
##
##  This program is free software: you can redistribute it and/or modify
##  it under the terms of the GNU General Public License as published by
##  the Free Software Foundation, either version 3 of the License, or
##  (at your option) any later version.
##
##  This program is distributed in the hope that it will be useful,
##  but WITHOUT ANY WARRANTY; without even the implied warranty of
##  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##  GNU General Public License for more details.
##
##  You should have received a copy of the GNU General Public License
##  along with this program.  If not, see <http://www.gnu.org/licenses/>.

import cairo
import math
import os
import sys

def make_image(width, height, text):
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    cr = cairo.Context(surface)

    # draw background
    cr.set_source_rgb(0.4, 0.4, 0.4)
    cr.rectangle(0, 0, width, height)
    cr.fill()

    # draw checkboard pattern
    cr.set_source_rgb(0.6, 0.6, 0.6)
    for y in range(0, 8):
        for x in range(0 + y%2, 8, 2):
            cr.rectangle(x * width/8, y * height/8, width/8, height/8)
            cr.fill()

    cr.set_source_rgba(1.0, 0.0, 0.0, 0.5)
    cr.arc(width/2, height/2, height * 0.4, 0, 2*math.pi)
    cr.fill()

    cr.set_source_rgb(1.0, 1.0, 0.0)
    cr.arc(width/2, height/2, height * 0.3, 0, 2*math.pi)
    cr.fill()

    cr.set_source_rgb(0.0, 1.0, 1.0)
    cr.arc(width/2, height/2, height * 0.15, 0, 2*math.pi)
    cr.fill()

    # outer circle
    cr.set_source_rgb(0.0, 0.0, 0.0)
    cr.arc(0, 0, 24, 0, 2*math.pi)
    cr.stroke()

    cr.arc(width, 0, 24, 0, 2*math.pi)
    cr.stroke()

    cr.arc(width, height, 24, 0, 2*math.pi)
    cr.stroke()

    cr.arc(0, height, 24, 0, 2*math.pi)
    cr.stroke()

    # inner circle
    cr.arc(0, 0, 12, 0, 2*math.pi)
    cr.fill()

    cr.arc(width, 0, 12, 0, 2*math.pi)
    cr.fill()

    cr.arc(width, height, 12, 0, 2*math.pi)
    cr.fill()

    cr.arc(0, height, 12, 0, 2*math.pi)
    cr.fill()

    cr.set_source_rgb(0.0, 0.0, 0.0)
    cr.select_font_face("Ubuntu", cairo.FONT_SLANT_NORMAL,
                        cairo.FONT_WEIGHT_BOLD)
    cr.set_font_size(24)
    cr.move_to(32, height - 12)
    cr.show_text(text)

    return surface


def film_countdown(width, height, second, frame, frames):
    surface = cairo.ImageSurface(cairo.FORMAT_ARGB32, width, height)
    cr = cairo.Context(surface)

    # background fill
    cr.set_source_rgb(0.5, 0.5, 0.5)
    cr.paint()

    # progress
    progress = (frame+1) / frames
    radius = width
    cr.move_to(width / 2, height / 2)
    cr.arc(width / 2, height / 2, radius,
           0 - math.pi/2, 2*math.pi * progress - math.pi/2)
    cr.close_path()

    cr.set_source_rgb(0.4, 0.4, 0.4)
    cr.fill_preserve()

    cr.set_source_rgb(0.25, 0.25, 0.25)
    cr.stroke()

    # background cross
    cr.move_to(width / 2, 0)
    cr.line_to(width / 2, height)
    cr.move_to(0, height / 2)
    cr.line_to(width, height / 2)

    cr.set_source_rgb(0.25, 0.25, 0.25)
    cr.set_line_width(6.0)
    cr.stroke()

    # two white circles
    radius = height / 2 * 0.9
    cr.arc(width / 2, height / 2, radius,
           0, 2*math.pi)
    cr.new_sub_path()
    cr.arc(width / 2, height / 2, radius * 0.8,
           0, 2*math.pi)

    cr.set_source_rgb(1, 1, 1)
    cr.set_line_width(6.0)
    cr.stroke()

    # progress
    cr.set_source_rgb(0, 0, 0)

    cr.select_font_face("Ubuntu",
                        cairo.FONT_SLANT_NORMAL,
                        cairo.FONT_WEIGHT_BOLD)
    cr.set_font_size(height * 0.8)
    center_text = "%d" % second
    x_bearing, y_bearing, text_width, text_height, x_advance, y_advance = cr.text_extents(center_text)
    cr.move_to(width / 2 - x_advance/2,
               height / 2 + text_height/2)
    cr.show_text(center_text)

    cr.set_font_size(height * 0.1)
    frameno_text = "%02d/%02d" % (frame+1, frames)
    x_bearing, y_bearing, text_width, text_height, x_advance, y_advance = cr.text_extents(frameno_text)
    cr.move_to(width  - x_advance - 8,
               height - 8)
    cr.show_text(frameno_text)

    return surface

def make_test_images(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

    for filename in ["out1.png", "out2.png"]:
        surface = make_image(514, 259, filename)
        outfile = os.path.join(directory, filename)
        surface.write_to_png(outfile)
        print("wrote %s" % outfile)

    for second in reversed(range(0, 4)):
        for frame in range(0, 24):
            surface = film_countdown(854, 480, second, frame, 24)
            outfile = os.path.join(directory, "countdown%02d-%03d.png" % (second, frame))
            surface.write_to_png(outfile)
            print("wrote %s" % outfile)


def main(args):
    for directory in args:
        make_test_images(directory)


if __name__ == "__main__":
    main(sys.argv[1:])


# EOF #
