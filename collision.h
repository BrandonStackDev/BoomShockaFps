#ifndef COLLISION_H
#define COLLISION_H

#include "raylib.h"
#include "level.h"

//constants for collision
#define SKIN_WIDTH 0.1f //0.05f

//enums
typedef enum {
    COLLISION_TOP,
    COLLISION_BOTTOM,
    COLLISION_SIDE,
    COLLISION_SIDE_IGNORE
} CollisionType;

//structs
typedef struct {
    CollisionType type;
    Vector3 v0;
    Vector3 v1;
    Vector3 v2;
    Vector3 normal;
    Vector3 center;
    float height;
} Collision;

//functions
void HandleObjectCollision(MainCharacter* mc, EnvObject* obj);
void HandleHitBoxesCollision(MainCharacter* mc, EnvObject* obj);
void HandleItemCollision(MainCharacter* mc, Item* item);
void HandleMcAndBgCollision(MainCharacter* mc, Enemy* bg, GameState *gs);
void HandleBgPlatCollision(Enemy* bg, Level* l);

#endif // COLLISION_H