#!/usr/bin/python

# import xattr
import sys
import os
import md5
import Image
from stat import *
from glob import fnmatch
import dircache

image_exts = [".jpg", ".png", ".JPG", ".JPEG", ".PNG"]

config_home = os.getenv("HOME")

if not config_home:
    print "Couldn't find $HOME environment variable"
    exit()
else:
    # FIXME: mkdir also 00-ff
    for hex in [a+b for a in "0123456789abcdef" for b in "0123456789abcdef"]:
        for org in ["by_url", "by_md5"]:
            for res in [2048, 1024, 512, 256, 128, 64, 32, 16]:
                dir = config_home + "/.griv/cache/%s/%d/%s" % (org, res, hex)
                if not os.access(dir, os.F_OK):
                    os.makedirs(dir)
                    print "Created", dir

def has_extension(filename, extensions):
    for ext in extensions:
        if filename.endswith(ext):
            return True
    return False

def md5sum(filename):
    f = open(filename)
    m = md5.new()
    m.update(f.read())
    f.close()
    return m.hexdigest()

def genthumb(orig_filename, img):
    global config_home
    for res in [2048, 1024, 512, 256, 128, 64, 32, 16]:
        url = "file://" + os.path.abspath(orig_filename)
        m = md5.new()
        m.update(url)
        url_md5 = m.hexdigest()

        url_filename = config_home + "/.griv/cache/by_url/%d/%s/%s.jpg" % (res, url_md5[0:2], url_md5[2:])

        if not os.path.exists(url_filename):
            if res < img.size[0] or res < img.size[1]: # don't thumbnail if orig is smaller
                if (img.size[0] > img.size[1]): # preserve aspect ratio
                    size = (res, res * img.size[1] / img.size[0])
                else:
                    size = (res * img.size[0] / img.size[1], res)

                img = img.resize(size, Image.BICUBIC) #
                img.save(url_filename, "JPEG", quality = 75)

                print "%4d %s => %s" % (res, orig_filename, url_filename)

def process_file(pathname):
    if os.path.isdir(pathname): 
        for i in [os.path.join(pathname, name) for name in os.listdir(pathname)]:
            process_file(i)        
    elif has_extension(pathname, image_exts):
        try:
            orig_img = Image.open(pathname)
            genthumb(pathname, orig_img)

        except IOError, err:
            print "Image Error:", pathname, err
    else:
        pass # ignore non-image files

def main(args):
    for pathname in args:
        process_file(pathname)

main(sys.argv[1:])

# EOF #
