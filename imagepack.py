#!/usr/bin/python

import struct
import sys
import os
import Image
import md5

scale = 16/16 # 1 == 16x16

def process_files(args):
    global scale 

    args.sort()

    if len(args) > 4096:
        print "Error: More then 4096 images given,", len(args)
        exit(0)

    print "."
    img = Image.new("RGB", (1024*scale, 1024*scale))
    print "."
    x = 0
    y = 0
    for pathname in args:
        try:
            # print "Processing: ", pathname
            source = Image.open(pathname)
            img.paste(source, (x,y))
            del source

            x += 16*scale
            if x >= 1024*scale:
                x = 0
                y += 16*scale
        except IOError, err:
            print pathname, err

    img.save("/tmp/out-%d.jpg" % (scale*16), "JPEG", quality = 75)

    # Generate index file
    f = open("/tmp/out-%d.idx" % (scale*16), "wb")
    f.write(struct.pack("I", len(args))) # FIXME: endianess

    for i in args: 
        f.write(os.path.basename(i))
        f.write("\0")
    f.close()

    print len(args), "images processed"


def gen_file_list(lst, pathname):
    if os.path.isdir(pathname): 
        for i in [os.path.join(pathname, name) for name in os.listdir(pathname)]:
            gen_file_list(lst, i)
    else:
        lst.append(pathname)
            
def main(args):
    lst = []
    for i in args:
        gen_file_list(lst, i)
    process_files(lst)

main(sys.argv[1:])

# EOF #
