#!/usr/bin/python

import xattr
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
    for res in [2048, 1024, 512, 256, 128, 64, 32, 16]:
        dir = config_home + "/.griv/%d" % res
        if not os.access(dir, os.F_OK):
            os.mkdir(dir)
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

def get_md5(filename):
    try:
        md5 = xattr.getxattr(filename, "user.griv.md5")
        
    except IOError, (errno, strerror):
        md5 = md5sum(filename)
        mtime = os.stat(filename)[ST_MTIME]
        xattr.setxattr(filename, "user.griv.md5", md5)
        xattr.setxattr(filename, "user.griv.mtime", "%d" % mtime)
        xattr.setxattr(filename, "user.griv.filter", "repeated-antialias")
        
    return md5

def genthumb(orig_filename, guid, img):
    global config_home
    for res in [2048, 1024, 512, 256, 128, 64, 32, 16]:
        filename = config_home + "/.griv/%d/%s.jpg" % (res, guid)
        if not os.path.exists(filename):
            if res < img.size[0] or res < img.size[1]:
                if (img.size[0] > img.size[1]):
                    size = (res, res * img.size[1] / img.size[0])
                else:
                    size = (res * img.size[0] / img.size[1], res)

                img = img.resize(size, Image.ANTIALIAS) # BICUBIC) #
                img.save(filename, "JPEG", quality = 75)

                print "%s %4d %s => %s" % (guid, res, orig_filename, filename)

def process_file(pathname):
    if os.path.isdir(pathname): 
        for i in [os.path.join(pathname, name) for name in os.listdir(pathname)]:
            process_file(i)        
    elif has_extension(pathname, image_exts):
        try:
            md5 = get_md5(pathname)
            orig_img = Image.open(pathname)
            genthumb(pathname, md5, orig_img)

        except IOError, err:
            print "Image Error:", pathname, err
    else:
        pass # ignore non-image files

def main(args):
    for pathname in args:
        process_file(pathname)

main(sys.argv[1:])

# EOF #
