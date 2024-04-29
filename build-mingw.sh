#!/bin/bash

rm -f visibility.exe
PIXEL_PER_DEGREE=4

C:/Users/Magenta/Downloads/w64devkit/bin/x86_64-w64-mingw32-gcc -static-libgcc -Wl,-Bstatic -fno-exceptions -DPIXEL_PER_DEGREE=$PIXEL_PER_DEGREE \
    -Wall -Werror -o visibility$PIXEL_PER_DEGREE.exe -fopenmp -O3 visibility.cc thirdparty/astronomy.c
C:/Users/Magenta/Downloads/w64devkit/bin/strip visibility$PIXEL_PER_DEGREE.exe
