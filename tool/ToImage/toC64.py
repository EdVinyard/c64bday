#!/usr/bin/env python3
'''
Convert a black and white image to a C array compatible with
the C64 marquee program, where each byte is an index into an
array of animation frames.

usage:

    python3 toC64.py marquee.gif > cc65_fragment.c

'''
import sys
from PIL import Image

def is_on(pixel): return pixel == 0
def is_off(pixel): return pixel == 255

EMPTY_BLOCK = 0
FULL_BLOCK = 8
EMPTY_TO_FULL = 16
FULL_TO_EMPTY = 24

filename = sys.argv[1]

# read the contents of the file
img = Image.open(filename)

for row in range(img.height):
    c_array = []
    for col in range(img.width):
        curr_pixel = img.getpixel( (col                , row) )
        next_pixel = img.getpixel( ((col+1) % img.width, row) )

        if is_off(curr_pixel) and is_off(next_pixel):
            c_array.append(EMPTY_BLOCK)
        elif is_on(curr_pixel) and is_on(next_pixel):
            c_array.append(FULL_BLOCK)
        elif is_off(curr_pixel) and is_on(next_pixel):
            c_array.append(EMPTY_TO_FULL)
        elif is_on(curr_pixel) and is_off(next_pixel):
            c_array.append(FULL_TO_EMPTY)

    c_array_str = ','.join( str(x) for x in c_array )
    print(f'const char row{row}[{img.width}] = {{ {c_array_str} }};')
