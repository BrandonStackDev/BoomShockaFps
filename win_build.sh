#!/bin/bash

gcc -DMEMORY_SAFE_MODE main.c level.c map_parser.c collision.c game.c functions.c timer.c -o game.exe -lraylib -lopengl32 -lgdi32 -lwinmm
