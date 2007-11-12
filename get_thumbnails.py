#!/usr/bin/python

import xattr
import sys
import os
import md5
import Image
from stat import *

def process_file(pathname):
    global res
    if os.path.isdir(pathname): 
        for i in [os.path.join(pathname, name) for name in os.listdir(pathname)]:
            process_file(i)        
    else:
        try:
            md5 = xattr.getxattr(pathname, "user.griv.md5")
            print "/home/ingo/.griv/%d/%s.jpg" % (res, md5)
        except IOError, (errno, stderr):
            pass

def main(args):
    for pathname in args:
        process_file(pathname)

res = int(sys.argv[1])
main(sys.argv[2:])

# EOF #
