#include "raylib.h"
#include "raymath.h"  // For vector calculations
#include "rlgl.h"
#include "level.h"
#include "game.h"
#include "collision.h"
#include <stdio.h>
#include <stdlib.h> 
#include <math.h>
#include <time.h>
#include <string.h>



// Internal: Get triangle edges
static inline Vector3 Vec3Sub(Vector3 a, Vector3 b) {
    return (Vector3){ a.x - b.x, a.y - b.y, a.z - b.z };
}

// Internal: Get triangle normal
static Vector3 TriangleNormal(Vector3 a, Vector3 b, Vector3 c) {
    Vector3 ab = Vec3Sub(b, a);
    Vector3 ac = Vec3Sub(c, a);
    return Vector3Normalize(Vector3CrossProduct(ab, ac));
}

// Internal: Project a point onto an axis
static float ProjectPoint(Vector3 p, Vector3 axis) {
    return Vector3DotProduct(p, axis);
}

// Internal: Project AABB onto an axis and get min/max
static void ProjectAABB(BoundingBox box, Vector3 axis, float *min, float *max) {
    Vector3 corners[8] = {
        box.min,
        {box.max.x, box.min.y, box.min.z},
        {box.min.x, box.max.y, box.min.z},
        {box.min.x, box.min.y, box.max.z},
        {box.max.x, box.max.y, box.min.z},
        {box.min.x, box.max.y, box.max.z},
        {box.max.x, box.min.y, box.max.z},
        box.max
    };

    *min = *max = ProjectPoint(corners[0], axis);
    for (int i = 1; i < 8; i++) {
        float proj = ProjectPoint(corners[i], axis);
        if (proj < *min) *min = proj;
        if (proj > *max) *max = proj;
    }
}

// Internal: Project triangle onto an axis and get min/max
static void ProjectTriangle(Vector3 v0, Vector3 v1, Vector3 v2, Vector3 axis, float *min, float *max) {
    *min = *max = ProjectPoint(v0, axis);
    float proj1 = ProjectPoint(v1, axis);
    float proj2 = ProjectPoint(v2, axis);

    if (proj1 < *min) *min = proj1;
    if (proj1 > *max) *max = proj1;
    if (proj2 < *min) *min = proj2;
    if (proj2 > *max) *max = proj2;
}

// SAT overlap check
static bool OverlapOnAxis(Vector3 v0, Vector3 v1, Vector3 v2, BoundingBox box, Vector3 axis) {
    if (axis.x == 0 && axis.y == 0 && axis.z == 0) return true; // ignore zero axis
    float minA, maxA, minB, maxB;
    ProjectTriangle(v0, v1, v2, axis, &minA, &maxA);
    ProjectAABB(box, axis, &minB, &maxB);
    return !(maxA < minB || maxB < minA);
}

// ✅ Main collision function
bool CheckTriangleAABBCollision(Vector3 v0, Vector3 v1, Vector3 v2, BoundingBox box) {
    // Triangle edges
    Vector3 e0 = Vec3Sub(v1, v0);
    Vector3 e1 = Vec3Sub(v2, v1);
    Vector3 e2 = Vec3Sub(v0, v2);

    // AABB axes
    Vector3 xAxis = {1, 0, 0};
    Vector3 yAxis = {0, 1, 0};
    Vector3 zAxis = {0, 0, 1};

    // 13 potential separating axes:
    Vector3 axes[13] = {
        xAxis, yAxis, zAxis,
        TriangleNormal(v0, v1, v2),
        Vector3CrossProduct(e0, xAxis),
        Vector3CrossProduct(e0, yAxis),
        Vector3CrossProduct(e0, zAxis),
        Vector3CrossProduct(e1, xAxis),
        Vector3CrossProduct(e1, yAxis),
        Vector3CrossProduct(e1, zAxis),
        Vector3CrossProduct(e2, xAxis),
        Vector3CrossProduct(e2, yAxis),
        Vector3CrossProduct(e2, zAxis)
    };

    // SAT test
    for (int i = 0; i < 13; i++) {
        if (!OverlapOnAxis(v0, v1, v2, box, axes[i])) return false;
    }

    return true; // No separating axis found → collision
}

// Given triangle (v0, v1, v2) and a mc X/Z, return interpolated Y on the triangle
bool GetTriangleHeightAtPosition(Vector3 v0, Vector3 v1, Vector3 v2, float x, float z, float *outY) {
    // Compute normal of the triangle (plane)
    Vector3 edge1 = Vector3Subtract(v1, v0);
    Vector3 edge2 = Vector3Subtract(v2, v0);
    Vector3 normal = Vector3CrossProduct(edge1, edge2);

    float A = normal.x;
    float B = normal.y;
    float C = normal.z;

    // If B == 0, triangle is vertical (can't solve for Y)
    if (fabsf(B) < 1e-6f) {
        // printf("Triangle rejected due to flat B: %f\n", B);
        // PrintVector3("v0", v0);
        // PrintVector3("v1", v1);
        // PrintVector3("v2", v2);
        return false;
    }

    // Solve plane equation for D
    float D = - (A * v0.x + B * v0.y + C * v0.z);

    // Solve for Y
    *outY = (-A * x - C * z - D) / B;
    return true;
}

Vector3 GetTrianglePitchDirection(Vector3 normal) {
    // Get the surface right vector (perpendicular to up and world forward)
    Vector3 worldForward = {0, 0, 1};
    Vector3 right = Vector3Normalize(Vector3CrossProduct(worldForward, normal));

    // Now get the actual forward vector along the triangle surface
    Vector3 surfaceForward = Vector3Normalize(Vector3CrossProduct(normal, right));
    return surfaceForward;
}

void GetSlopeOrientation(Vector3 normal, Vector3 *outRight, Vector3 *outForward) {
    Vector3 worldForward = { 0, 0, 1 };

    // Surface right = perpendicular to surface normal and world forward
    *outRight = Vector3Normalize(Vector3CrossProduct(worldForward, normal));

    // Surface forward = perpendicular to surface normal and surface right
    *outForward = Vector3Normalize(Vector3CrossProduct(normal, *outRight));
}

float GetPitchFromForward(Vector3 forward) {
    return -asinf(forward.y); // Negative for downhill = positive pitch
}

float GetRollFromRight(Vector3 right) {
    return -asinf(right.y); // Negative for right side down = positive roll
}

static Vector3 GetCenterForTriangle(Vector3 v0, Vector3 v1, Vector3 v2)
{
    Vector3 center = {
        (v0.x + v1.x + v2.x) / 3.0f,
        (v0.y + v1.y + v2.y) / 3.0f,
        (v0.z + v1.z + v2.z) / 3.0f
    };
    return center;
}

void HandleObjectCollision(MainCharacter* mc, EnvObject* obj)
{
    int triangleCount = obj->model.meshes[0].triangleCount;
    float *vertices = obj->model.meshes[0].vertices;

    mc->isOnPlatform = false;
    bool foundGround = false;
    float bestGroundY = -INFINITY;
    float wallY = 0.0f;
    bool hitCeiling = false;
    bool hitSide = false;
    Collision colls[triangleCount];
    int collCount = 0;
    float maxObjectHeight = -INFINITY;
    float minObjectHeight = INFINITY;

    for (int i = 0; i < triangleCount; i++) {
        Vector3 v0 = {
            vertices[(i * 3 + 0) * 3 + 0],
            vertices[(i * 3 + 0) * 3 + 1],
            vertices[(i * 3 + 0) * 3 + 2]
        };
        Vector3 v1 = {
            vertices[(i * 3 + 1) * 3 + 0],
            vertices[(i * 3 + 1) * 3 + 1],
            vertices[(i * 3 + 1) * 3 + 2]
        };
        Vector3 v2 = {
            vertices[(i * 3 + 2) * 3 + 0],
            vertices[(i * 3 + 2) * 3 + 1],
            vertices[(i * 3 + 2) * 3 + 2]
        };
        
        float maxTriY = fmaxf(v0.y, fmaxf(v1.y, v2.y));
        float minTriY = fminf(v0.y, fminf(v1.y, v2.y));
        if(maxObjectHeight < maxTriY){maxObjectHeight = maxTriY;}
        if(minObjectHeight > minTriY){minObjectHeight = minTriY;}

        if (!CheckTriangleAABBCollision(v0, v1, v2, mc->box)) 
        {
            continue; //no collision
        }

        Vector3 normal = TriangleNormal(v0, v1, v2);
        float dotUp = Vector3DotProduct(normal, (Vector3){0, 1, 0});
        //printf("dotUp: %f\n",dotUp);
        if (GetTriangleHeightAtPosition(v0, v1, v2, mc->pos.x, mc->pos.z, &wallY)) {
            if ((mc->oldPos.y >= wallY && mc->pos.y <= wallY) || dotUp > 0.7f) {
                // Ground
                if (wallY > bestGroundY && mc->pos.y - wallY < 0.25f) {
                    foundGround = true;
                    bestGroundY = wallY;
                    colls[collCount] = (Collision) {COLLISION_TOP,v0,v1,v2,normal,GetCenterForTriangle(v0,v1,v2), wallY};
                    collCount++;
                }
            }
            else if (dotUp < -0.7f) {
                // Ceiling
                hitCeiling = true;
                colls[collCount] = (Collision) {COLLISION_BOTTOM,v0,v1,v2,normal,GetCenterForTriangle(v0,v1,v2), wallY};
                collCount++;
            }
            else {
                hitSide = true;
                colls[collCount] = (Collision) {COLLISION_SIDE,v0,v1,v2,normal,GetCenterForTriangle(v0,v1,v2), wallY};
                collCount++;
            }
        }
        else {
            if (fabsf(maxTriY - mc->pos.y) > 0.01f) {
                //printf("vertical triangle, far from y position\n");
                hitSide = true;
                colls[collCount] = (Collision) {COLLISION_SIDE_IGNORE,v0,v1,v2,normal,GetCenterForTriangle(v0,v1,v2), wallY};
                collCount++;
            }
        }
    }

    bool wasJumping = mc->isJumping;
    if ((foundGround && mc->yVelocity <= 0 && fabsf(bestGroundY - mc->pos.y) < 0.4)
        || (foundGround && !wasJumping && mc->camera.position.y > maxObjectHeight)) //only sets y
    {  
        //printf("Collision type: COLLISION_TOP\n");
        if(!hitCeiling)
        {
            //printf("no ceiling, applying y change, %f, %f\n", mc->pos.y, bestGroundY);
            mc->pos.y = bestGroundY;
        }
        mc->yVelocity = 0;
        mc->isJumping = false;
        mc->isFalling = false;
        mc->isOnPlatform = true;
    }
    if ((hitCeiling && !hitSide)||(hitCeiling && wasJumping)) {//ugh
        //printf("Collision type: COLLISION_BOTTOM\n");
        //mc->yVelocity = 0;
        if(!mc->isCrouching && !wasJumping && !foundGround){mc->isCrouching = true;}//auto crouch, like if he stands up in a tunnel
        else if(wasJumping && mc->yVelocity >= 0)
        {
            float newY = minObjectHeight - SKIN_WIDTH;
            //printf("was jumping, bottom y change suggested: %f, %f\n", mc->pos.y, newY);
            if(newY < mc->pos.y)
            {
                //printf("bottom y change accepted: %f\n", newY);
                mc->pos.y = newY;
            }//gotta check if this moves us down
            else
            {
                mc->pos.y -= SKIN_WIDTH;//move him down anyway
                //printf("bottom y change auto: %f\n", mc->pos.y);
            }
            mc->isJumping = false;
            mc->isFalling = true;
            mc->isOnPlatform = false;
        }
    }
    if (hitSide) {//only sets x and z
        //printf("Collision type: COLLISION_SIDE\n");
        for(int i = 0; i < collCount; i++)
        {
            if(colls[i].type == COLLISION_SIDE 
                || (colls[i].type == COLLISION_SIDE_IGNORE && !foundGround)
                || (colls[i].type == COLLISION_SIDE_IGNORE && foundGround && maxObjectHeight > (mc->crouchHeight + mc->pos.y))
            )//double ugh
            {
                Vector3 movement = Vector3Subtract(mc->pos, mc->oldPos);
                Vector3 slide = Vector3Subtract(movement, Vector3Scale(colls[i].normal, Vector3DotProduct(movement, colls[i].normal)));
                Vector3 candidatePos = Vector3Add(mc->oldPos, slide);

                BoundingBox testBox = mc->box;
                Vector3 offset = Vector3Subtract(candidatePos, mc->pos);
                testBox.min = Vector3Add(testBox.min, offset);
                testBox.max = Vector3Add(testBox.max, offset);

                if (!CheckTriangleAABBCollision(colls[i].v0, colls[i].v1, colls[i].v2, testBox)) {
                    mc->pos.x = candidatePos.x;
                    mc->pos.z = candidatePos.z;
                    //printf("Slide Accepted\n");// Slide accepted
                } else {
                    // Push out slightly along the wall normal to avoid getting stuck
                    const float pushOutDist = 0.01f; // tweak if needed
                    Vector3 pushOut = Vector3Scale(colls[i].normal, pushOutDist);

                    Vector3 tryPos = Vector3Add(mc->oldPos, pushOut);
                    BoundingBox testBox2 = mc->box;
                    Vector3 offset2 = Vector3Subtract(tryPos, mc->pos);
                    testBox2.min = Vector3Add(testBox2.min, offset2);
                    testBox2.max = Vector3Add(testBox2.max, offset2);

                    if (!CheckTriangleAABBCollision(colls[i].v0, colls[i].v1, colls[i].v2, testBox2)) {
                        mc->pos.x = tryPos.x;
                        mc->pos.z = tryPos.z;
                        //printf("Wall push-out applied\n");
                    } else {
                        Vector3 movement = Vector3Subtract(mc->pos, mc->oldPos);
                        float dot = Vector3DotProduct(movement, colls[i].normal);
                        if (dot > 0.0f) {
                            // Moving away from wall accept movement
                            mc->pos.x = tryPos.x; // already updated
                            mc->pos.z = tryPos.z;
                            //printf("Slide blocked, but pushout accepted because moving away from wall\n");
                        } else {
                            // Moving into wall revert
                            mc->pos.x = mc->oldPos.x;
                            mc->pos.z = mc->oldPos.z;
                            //printf("Slide + push-out blocked, moving into wall\n");
                        } 
                    }
                }
            }
        }
    }
}

// void HandleHitBoxesCollision(MainCharacter* mc, EnvObject* obj)
// {
//     if(!obj->useHitBoxes){return;}
//     for(int i=0; i<obj->hitBoxCount; i++)
//     {
//         if(CheckCollisionBoxes(mc->box, obj->hitBoxes[i]))
//         {
//             mc->pos.x = mc->oldPos.x;
//             mc->pos.z = mc->oldPos.z;
//         }
//     }
// }

void HandleHitBoxesCollision(MainCharacter* mc, EnvObject* obj)
{
    if (!obj->useHitBoxes) return;

    BoundingBox playerBox = mc->box;

    for (int i = 0; i < obj->hitBoxCount; i++)
    {
        BoundingBox hitBox = obj->hitBoxes[i];

        if (CheckCollisionBoxes(playerBox, hitBox))
        {
            Vector3 oldPos = mc->oldPos;
            Vector3 newPos = mc->pos;

            // Determine penetration depth on each axis
            float dx1 = hitBox.max.x - playerBox.min.x; // penetration from left
            float dx2 = playerBox.max.x - hitBox.min.x; // penetration from right
            float dz1 = hitBox.max.z - playerBox.min.z; // from front
            float dz2 = playerBox.max.z - hitBox.min.z; // from back
            float dy1 = hitBox.max.y - playerBox.min.y; // from below (top collision)
            float dy2 = playerBox.max.y - hitBox.min.y; // from above (bottom collision)

            float penX = (dx1 < dx2) ? dx1 : -dx2;
            float penZ = (dz1 < dz2) ? dz1 : -dz2;
            float penY = (dy1 < dy2) ? dy1 : -dy2;

            // Resolve collision in the axis of least penetration
            float absX = fabsf(penX);
            float absY = fabsf(penY);
            float absZ = fabsf(penZ);

            if (absY < absX && absY < absZ)
            {
                mc->pos.y += penY;

                // Land on top
                if (penY < 0)
                {
                    mc->yVelocity = 0;
                    mc->isJumping = false;
                    mc->isFalling = false;
                    mc->isOnPlatform = true;
                }
            }
            else if (absX < absZ)
            {
                mc->pos.x += penX;
            }
            else
            {
                mc->pos.z += penZ;
            }

            // Update bounding box to reflect new position
            mc->box = UpdateBoundingBox(mc->isCrouching?mc->originalCrouchBox:mc->originalBox,mc->pos);
        }
    }
}

void HandleItemCollision(MainCharacter* mc, Item* item)
{
    if(CheckCollisionBoxes(mc->box, item->box))
    {
        item->isCollected = true;
        if(item->type==ITEM_M1GRAND)
        {
            mc->hasAnyWeapon = true;
            mc->curWeaponIndex = WEAPON_M1GRAND;
            mc->weapons[WEAPON_M1GRAND].have = true;
        }
        else if(item->type==ITEM_AMMO_M1GRAND)
        {
            if(mc->hasAnyWeapon && mc->weapons[WEAPON_M1GRAND].have)
            {
                mc->weapons[WEAPON_M1GRAND].ammo += 25; //todo: max ammo? reload?
            }
            else {item->isCollected = false;} //only collect if you have m1grand
        }
        else if(item->type==ITEM_HEALTH)
        {
            if(mc->health < mc->maxHealth)
            {
                mc->health = mc->maxHealth; //todo: limit this?
            }
            else {item->isCollected = false;} //only collect if you have need of health
        }
    }
}
/// @brief this shouldnt happen much, very simple for now
void HandleMcAndBgCollision(MainCharacter* mc, Enemy* bg, GameState *gs)
{
    if(CheckCollisionBoxes(mc->box, bg->box))
    {
        // Compute a push direction: from yeti to player
        Vector3 pushDir = Vector3Subtract(mc->pos, bg->pos);

        // Avoid division by zero
        if (Vector3Length(pushDir) < 0.001f)
        {
            pushDir = (Vector3){1.0f, 0.0f, 0.0f}; // arbitrary direction
        }

        pushDir = Vector3Normalize(pushDir);

        // Push the player slightly away from the yeti (adjust strength as needed)
        float pushDistance = 0.1f; // tweak this if needed
        mc->pos.x += pushDir.x * pushDistance;
        mc->pos.z += pushDir.z * pushDistance;
        if(bg->type==BG_TYPE_YETI && !gs->t_collDamage_wait.wasStarted)
        {
            mc->health -= 10;
            StartTimer(&gs->t_collDamage_wait);
        }
        mc->box = UpdateBoundingBox(mc->isCrouching?mc->originalCrouchBox:mc->originalBox,mc->pos);
    }
}
//todo: does this actually work? with ramps?
void HandleBgPlatCollision(Enemy* bg, Level* l)
{
    BoundingBox playerBox = bg->box;
    bool isOnPlatform = false;
    bool sideColl = false;
    for(int i=0;i<l->objCount;i++)
    {
        BoundingBox hitBox = l->obj[i].box;
        //printf("bg plat begin %d\n", i);
        if(CheckCollisionBoxes(playerBox,hitBox))
        {
            //do stuff
            Vector3 oldPos = bg->oldPos;
            Vector3 newPos = bg->pos;

            // Determine penetration depth on each axis
            float dx1 = hitBox.max.x - playerBox.min.x; // penetration from left
            float dx2 = playerBox.max.x - hitBox.min.x; // penetration from right
            float dz1 = hitBox.max.z - playerBox.min.z; // from front
            float dz2 = playerBox.max.z - hitBox.min.z; // from back
            float dy1 = hitBox.max.y - playerBox.min.y; // from below (top collision)
            float dy2 = playerBox.max.y - hitBox.min.y; // from above (bottom collision)

            float penX = (dx1 < dx2) ? dx1 : -dx2;
            float penZ = (dz1 < dz2) ? dz1 : -dz2;
            float penY = (dy1 < dy2) ? dy1 : -dy2;

            // Resolve collision in the axis of least penetration
            float absX = fabsf(penX);
            float absY = fabsf(penY);
            float absZ = fabsf(penZ);

            if (absY < absX && absY < absZ)
            {
                //printf("bg plat vertical collision\n");
                //bg->pos.y += penY;
                if(bg->type==BG_TYPE_YETI && bg->isJumping && !l->mc.isJumping)
                {
                    if(Vector3Distance(l->mc.pos, bg->pos) < YETI_IMPACT_RADIUS)
                    {
                        l->mc.health-=5;
                        l->mc.isCrouching = true;
                    }
                }
                bg->pos.y = hitBox.max.y + bg->yOffset;
                bg->yVelocity = 0;
                bg->isJumping = false;
                bg->isFalling = false;
                isOnPlatform = true;
            }
            else if (absX < absZ)
            {
                //printf("bg plat side x xollision\n");
                bg->pos.x += penX;
                if(l->obj[i].type == OBJECT_OTHER){sideColl = true;}
            }
            else
            {
                //printf("bg plat side z collision\n");
                bg->pos.z += penZ;
                if(l->obj[i].type == OBJECT_OTHER){sideColl = true;}
            }

            // Update bounding box to reflect new position
            bg->box = UpdateBoundingBox(bg->origBox,bg->pos);
        }
    }
    if(!isOnPlatform)
    {
        bg->isFalling = true;
        if(bg->type==BG_TYPE_ARMY && bg->state==BG_STATE_WALKING)//I have isFalling, but he should never fall off the platform this way
        {
            bg->pos = bg->oldPos;
            bg->state=BG_STATE_SHOOTING;
            bg->anim=ANIM_SHOOT;
            bg->curFrame = 0;
        }
    }
    if(sideColl && bg->state == BG_STATE_WALKING)
    {
        // Direction away from wall
        Vector3 bounceDir = Vector3Normalize(Vector3Subtract(bg->oldPos, bg->pos));
        // New run target 2.5 units away from wall
        bg->targetPos = Vector3Add(bg->pos, Vector3Scale(bounceDir, 2.5f));
    }

    bg->box = UpdateBoundingBox(bg->origBox,bg->pos);//one more time for good luck...
    bg->headBox = UpdateBoundingBox(bg->origHeadBox,bg->pos);//two more time for good luck...
    bg->bodyBox = UpdateBoundingBox(bg->origBodyBox,bg->pos);//three more time for good luck!
}

