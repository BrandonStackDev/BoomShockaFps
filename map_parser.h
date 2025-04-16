#ifndef MAP_PARSER_H
#define MAP_PARSER_H

#include "raylib.h"


typedef struct {
    Model model;
    char className[64];
    bool hasOrigin;
    Vector3 origin;
    bool hasSubType;
    char subType[64];
} Entity;

Entity* LoadMapFile(const char *filename, int *modelCount);

#endif
