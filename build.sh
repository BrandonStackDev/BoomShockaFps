#!/bin/bash

gcc main.c level.c map_parser.c collision.c game.c functions.c timer.c -o game -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
