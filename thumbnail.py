#!/usr/bin/python

import xattr
import sys
import os
import md5
import Image
from stat import *
import dircache

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
        xattr.setxattr(filename, "user.griv.filter", "BICUBIC")
        
    return md5

def genthumb(orig_filename, guid, img, res):
    filename = "/home/ingo/.griv/%d/%s.jpg" % (res, guid)
    if not os.path.exists(filename):
        if res < img.size[0] or res < img.size[1]:
            if (img.size[0] > img.size[1]):
                size = (res, res * img.size[1] / img.size[0])
            else:
                size = (res * img.size[0] / img.size[1], res)

            new_img = img.resize(size, Image.BICUBIC) #ANTIALIAS)
            new_img.save(filename, "JPEG", quality = 75)

            print "%s %4d %s => %s" % (guid, res, orig_filename, filename)

def process_file(pathname):
    if os.path.isdir(pathname): 
        for i in [os.path.join(pathname, name) for name in os.listdir(pathname)]:
            process_file(i)        
    else:
        try:
            md5 = get_md5(pathname)

            orig_img = Image.open(pathname)
            for res in [16, 32, 64, 128, 256, 512, 1024, 2048]:
                genthumb(pathname, md5, orig_img, res)

        except IOError, err:
            print "Image Error:", err

def main(args):
    for pathname in args:
        process_file(pathname)

main(sys.argv[1:])

# EOF #
