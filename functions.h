#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "level.h"
#include "game.h"
#include "raylib.h"


//structs
typedef struct Plane {
    Vector3 normal;
    float d;
} Plane;

typedef struct Frustum {
    Plane planes[6]; // left, right, top, bottom, near, far
} Frustum;

//functions
void DrawTriangles(Model *m, bool useOrigin, Vector3 origin);
void DrawTrianglesIndexed(Model *m, bool useOrigin, Vector3 origin);
bool IsModelIndexed(Model *m);
void DrawHealthBar(Vector2 position, float width, float height, float healthPercent);
void DrawCrosshair();
void DrawGunHeld(Model gunModel, Camera camera, Vector3 gunPos, float rot);
void ShootRay(Level *l);
float RandRange(float min, float max);
Vector3 GetRandomRunTarget(Vector3 origin, float minDist, float maxDist);
float GetYawToTarget(Vector3 from, Vector3 to);
void HandleBgShotPlayer(Level *l, GameState *gs, int enemyIndex);
void HandleBgState(MainCharacter *mc, Enemy *bg);
void HandleBgArmyAnimEnd(Enemy *bg, Level *l, GameState *gs, int i);
void HandleBgYetiAnimEnd(Enemy *bg,Level *l, GameState *gs);
bool IsWithinDistance(Vector3 a, Vector3 b, float maxDist);
Frustum ExtractFrustum(Matrix mat);
bool IsBoxInFrustum(BoundingBox box, Frustum frustum);
void DrawCustomFPS(int x, int y, Color color);
void DrawHeart(Vector2 position, float size, Color color);

#endif // FUNCTIONS_H