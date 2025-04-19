#!/bin/bash

source ../emsdk/emsdk_env.sh
export PATH=$HOME/binaryen/build/bin:$PATH

emcc -o game.html main.c level.c map_parser.c collision.c game.c functions.c timer.c -I../raylib/src -L../raylib/src -lraylib -s USE_GLFW=3 -s USE_WEBGL2=0 -s ASYNCIFY -s FORCE_FILESYSTEM=1 -s TOTAL_MEMORY=67108864 -s STACK_SIZE=4194304 -s ALLOW_MEMORY_GROWTH=1 -s FULL_ES3=0 -s ASSERTIONS=2 -gsource-map --source-map-base http://localhost:8000/ --preload-file models --preload-file maps --preload-file textures -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 --shell-file ../raylib/src/shell.html