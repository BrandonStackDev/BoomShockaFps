#!/bin/bash

set -e

OUT_UNIVERSAL="game"
OUT_X86_64="game_x86_64"
OUT_ARM64="game_arm64"

CFLAGS="-I/usr/local/include -DPLATFORM_DESKTOP"
RAYLIB_STATIC="/usr/local/lib/libraylib.a"
LDFLAGS="$RAYLIB_STATIC -lm \
  -framework OpenGL -framework Cocoa -framework IOKit -framework CoreAudio -framework CoreVideo"

echo "Building x86_64..."
clang -arch x86_64 $CFLAGS *.c -o $OUT_X86_64 $LDFLAGS

echo "Building arm64..."
clang -arch arm64 $CFLAGS *.c -o $OUT_ARM64 $LDFLAGS

echo "Creating universal binary..."
lipo -create -output $OUT_UNIVERSAL $OUT_X86_64 $OUT_ARM64

echo "Built universal binary: $OUT_UNIVERSAL"
file $OUT_UNIVERSAL
