#include "level.h"
#include "game.h"
#include "timer.h"
#include "collision.h"
#include "functions.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>   // for sinf, cosf
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void DrawTriangles(Model *m, bool useOrigin, Vector3 origin)
{
    for(int j=0; j<m->meshCount; j++)
    {
        int triangleCount = m->meshes[j].triangleCount;
        float *vertices = m->meshes[j].vertices;
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
            if(useOrigin)
            {
                v0 = Vector3Add(v0,origin);
                v1 = Vector3Add(v1,origin);
                v2 = Vector3Add(v2,origin);
            }
            //DrawTriangle3D(v0,v1,v2,RED);
            DrawLine3D(v0,v1,RED);
            DrawLine3D(v1,v2,RED);
            DrawLine3D(v2,v0,RED);
        }
    }
}

void DrawTrianglesIndexed(Model *m, bool useOrigin, Vector3 origin)
{
    for(int j=0; j<m->meshCount; j++)
    {
        int triangleCount = m->meshes[j].triangleCount;
        float *vertices = m->meshes[j].vertices;
        unsigned short *indices = m->meshes[j].indices;  // Check the format if not ushort

        for (int i = 0; i < triangleCount; i++) {
            int i0 = indices[i * 3 + 0];
            int i1 = indices[i * 3 + 1];
            int i2 = indices[i * 3 + 2];

            Vector3 v0 = {
                vertices[i0 * 3 + 0],
                vertices[i0 * 3 + 1],
                vertices[i0 * 3 + 2]
            };
            Vector3 v1 = {
                vertices[i1 * 3 + 0],
                vertices[i1 * 3 + 1],
                vertices[i1 * 3 + 2]
            };
            Vector3 v2 = {
                vertices[i2 * 3 + 0],
                vertices[i2 * 3 + 1],
                vertices[i2 * 3 + 2]
            };

            if (useOrigin) {
                v0 = Vector3Add(v0, origin);
                v1 = Vector3Add(v1, origin);
                v2 = Vector3Add(v2, origin);
            }

            DrawLine3D(v0, v1, RED);
            DrawLine3D(v1, v2, RED);
            DrawLine3D(v2, v0, RED);
        }
    }
}

bool IsModelIndexed(Model *m)
{
    return m->meshes[0].vertexCount != (m->meshes[0].triangleCount * 3)||m->meshes[0].indices != NULL;
}

void DrawHealthBar(Vector2 position, float width, float height, float healthPercent)
{
    // Background (empty bar)
    DrawRectangle(position.x, position.y, width, height, DARKGRAY);
    // Foreground (current health)
    DrawRectangle(position.x, position.y, width * healthPercent, height, healthPercent < 0.33 ? RED : GREEN);
}

void DrawCrosshair()
{
    int centerX = SCREEN_WIDTH / 2;
    int centerY = SCREEN_HEIGHT / 2;
    int size = 10;  // Length of each line from center
    DrawLine(centerX - size, centerY, centerX + size, centerY, RAYWHITE); // Horizontal
    DrawLine(centerX, centerY - size, centerX, centerY + size, RAYWHITE); // Vertical
}

void DrawGunHeld(Model gunModel, Camera camera, Vector3 gunPos, float rot)
{
    // Create a fake FPS-style camera pointing forward
    Camera gunCam = {
        .position = (Vector3){ 0, 0, 0 },
        .target = (Vector3){ 0, 0, 1 },
        .up = (Vector3){ 0, 1, 0 },
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE
    };

    // Use BeginMode3D with gun camera
    BeginMode3D(gunCam);
        // Offset the gun slightly in front of camera origin
        rlPushMatrix();
        rlTranslatef(gunPos.x, gunPos.y, gunPos.z);
        rlRotatef(rot, 0, 1, 0);//Rotate 90 degrees around Y to face forward (+Z)
        DrawModel(gunModel, (Vector3){ 0 }, 1, WHITE);
        rlPopMatrix();
    EndMode3D();
}

void ShootRay(Level *l)
{
    // Step 1: Create a ray from the camera
    Vector3 origin = l->mc.camera.position;
    Vector3 direction = Vector3Normalize(Vector3Subtract(l->mc.camera.target, l->mc.camera.position));
    Ray bulletRay = (Ray){ origin, direction };
    bool hit = false;
    bool headShot = false;
    bool bodyShot = false;
    float minDistance = INFINITY;
    int minIndex = 0;
    float points = 0;
    // Step 2: Check for hit against all bad guys
    for (int i = 0; i < l->bgCount; i++)
    {
        if (!l->bg[i].dead && l->bg[i].state != BG_STATE_DYING)
        {
            RayCollision coll = GetRayCollisionBox(bulletRay, l->bg[i].box);
            RayCollision collBody = GetRayCollisionBox(bulletRay, l->bg[i].bodyBox);
            RayCollision collHead = GetRayCollisionBox(bulletRay, l->bg[i].headBox);
            //todo: collision detect walls with shorter distance collsions, ie. I can shoot through walls right now
            if (coll.hit && coll.distance < l->mc.weapons[l->mc.curWeaponIndex].maxDist)
            {
                printf("Bad guy %d hit, not yet accepted, distance %f\n", i, coll.distance);
                if(minDistance > coll.distance)
                {
                    minDistance = coll.distance;minIndex = i;
                    hit=true;
                    bodyShot = collBody.hit;
                    headShot = collHead.hit;
                }
            }
            else if (coll.hit && coll.distance >= l->mc.weapons[l->mc.curWeaponIndex].maxDist){printf("Bad guy %d out of range!\n", i);}
        }
    }
    if(hit)
    {
        for(int i=0; i<l->objCount; i++)
        {
            if(l->obj[i].useHitBoxes)
            {
                for(int j=0; j<l->obj[i].hitBoxCount; j++)
                {
                    RayCollision coll = GetRayCollisionBox(bulletRay, l->obj[i].hitBoxes[j]);
                    if(coll.hit && coll.distance < minDistance){printf("hit obj hit-box, %d, %d\n", i, j);return;} 
                }
            }
            else
            {
                RayCollision coll = GetRayCollisionBox(bulletRay, l->obj[i].box);
                if(coll.hit && coll.distance < minDistance){printf("hit wall %d\n", i);return;}
            }
        }
        printf("Bad guy %d hit!\n", minIndex);
        if(bodyShot){printf("Bad guy %d Body Shot!\n", minIndex);points=10+minDistance;}
        else if(!headShot){printf("Enemy %d grazed ... ?\n", minIndex);points=2;}
        if(headShot){printf("Bad guy %d HEAD_SHOT, SNIPER!\n", minIndex);points=(10 * minDistance);}
        float damn = bodyShot ? l->mc.weapons[l->mc.curWeaponIndex].damage : 2;
        if(headShot && l->bg[minIndex].type==BG_TYPE_YETI){damn=50;}
        l->bg[minIndex].health -= damn; // subtract hit
        l->mc.score += ((int) points);
        l->bg[minIndex].state = BG_STATE_HIT;
        l->bg[minIndex].anim = l->bg[minIndex].type==BG_TYPE_ARMY?ANIM_HIT:ANIM_YETI_ROAR;
        l->bg[minIndex].curFrame = l->bg[minIndex].type==BG_TYPE_ARMY?60:0; //hit anim is delayed with standing still, skip ahead unless yeti
        if(l->bg[minIndex].health <= 0 || (l->bg[minIndex].type==BG_TYPE_ARMY && headShot))//bg is dead
        {
            l->bg[minIndex].health = 0;
            l->bg[minIndex].state = BG_STATE_DYING;
            l->bg[minIndex].anim = l->bg[minIndex].type==BG_TYPE_ARMY?ANIM_DEATH:ANIM_YETI_ROAR;
            l->bg[minIndex].curFrame = 0;
            StartTimer(&l->bg[minIndex].t_yeti_death_wait);
            printf("Bad guy %d dead!\n", minIndex);
            if(!IsSoundPlaying(l->bg[minIndex].deathSound)){PlaySound(l->bg[minIndex].deathSound);}
        }//mark if dying as needed
        else //if hit but still alive
        {
            if(!IsSoundPlaying(l->bg[minIndex].hitSound)){PlaySound(l->bg[minIndex].hitSound);}
        }
    }
}

float RandRange(float min, float max)
{
    return min + (float)rand() / (float)RAND_MAX * (max - min);
}

Vector3 GetRandomRunTarget(Vector3 origin, float minDist, float maxDist) {
    float angle = RandRange(0.0f, 2.0f * PI); // Random angle in radians
    float distance = RandRange(minDist, maxDist);

    Vector3 offset = {
        cosf(angle) * distance,
        0.0f,  // Assuming flat terrain for now
        sinf(angle) * distance
    };

    return Vector3Add(origin, offset);
}

float GetYawToTarget(Vector3 from, Vector3 to)
{
    Vector3 dir = Vector3Subtract(to, from);
    return atan2f(dir.x, dir.z);  // Note: Z is forward in Raylib
}

void HandleBgShotPlayer(Level *l, GameState *gs, int enemyIndex)
{
    // Step 1: Create a ray from the camera and detect collision (should always hit)
    Vector3 origin = l->bg[enemyIndex].pos;
    Vector3 direction = Vector3Normalize(Vector3Subtract(l->mc.pos, l->bg[enemyIndex].pos));
    Ray bulletRay = (Ray){ origin, direction };
    RayCollision mcColl = GetRayCollisionBox(bulletRay, l->mc.box);
    //report if its not a hit because that is strange
    if(!mcColl.hit){printf("HandleBgShotPlayer, no hit on MC?\n");}
    // step 2: check if collision hits walls instead
    for(int i=0; i<l->objCount; i++)
    {
        if(l->obj[i].useHitBoxes)
        {
            for(int j=0; j<l->obj[i].hitBoxCount; j++)
            {
                RayCollision coll = GetRayCollisionBox(bulletRay, l->obj[i].hitBoxes[j]);
                if(coll.hit && coll.distance < mcColl.distance)
                {
                    //printf("bg %d hit obj hit-box, %d, %d\n", enemyIndex, i, j);
                    return;
                } 
            }
        }
        else
        {
            RayCollision coll = GetRayCollisionBox(bulletRay, l->obj[i].box);
            if(coll.hit && coll.distance < mcColl.distance)
            {
                //printf("bg %d hit wall %d\n", enemyIndex, i);
                return;
            }
        }
    }
    float damage = 5;
    int thresh = 3;
    if(gs->diff == DIFFICULTY_NORMAL){damage *= 2; thresh=2;}
    else if(gs->diff == DIFFICULTY_HARD){damage *= 3; thresh=1;}
    if(RandRange(0,4)>thresh){l->mc.health-=damage;}//todo: sound
}

void HandleBgState(Level *l, MainCharacter *mc, Enemy *bg, int index)
{
    if(bg->dead){return;}
    if(bg->state == BG_STATE_STILL && Vector3Distance(mc->pos, bg->pos) < BG_TO_MC_WAKE_UP_DIST)
    {
        bg->state = BG_STATE_PLANNING;
    }
    else if(bg->state == BG_STATE_PLANNING)
    {
        bool los = BgLineOfSightToMc(l,bg,index);
        if(Vector3Distance(mc->pos, bg->pos) > BG_TO_MC_WAKE_UP_DIST)
        {
            bg->state = BG_STATE_STILL;
            return;
        }
        if(bg->type==BG_TYPE_ARMY)
        {
            bg->targetPos = bg->isShooter?mc->pos:GetRandomRunTarget(bg->pos, 4, 10);//needed otherwise they spin sometimes
            bg->state = BG_STATE_WALKING;
            bg->anim = ANIM_WALKING;
            if(bg->isShooter && los)//shooter stands his ground, no walking, just shoot
            {
                bg->state = BG_STATE_SHOOTING;
                bg->anim = ANIM_SHOOT;
            }
            else if(bg->isShooter)
            {
                bg->state = BG_STATE_PLANNING;
                return;
            }//for shooters that dont have line of sight, skip the yaw stuff, otherwise, sometimes they spin
        }
        else if(bg->type==BG_TYPE_YETI)
        {
            bg->targetPos = mc->pos;
            bg->state = BG_STATE_WALKING;
            bg->anim = ANIM_YETI_WALK;  
        }
        bg->curFrame = 0;
        bg->yaw = GetYawToTarget(bg->pos,bg->targetPos);
        StartTimer(&bg->t_walk_stuck);
    }
    else if(bg->state == BG_STATE_WALKING)
    {
        Vector3 direction = Vector3Subtract(bg->targetPos, bg->pos);
        direction = Vector3Scale(Vector3Normalize(direction), bg->speed * GetFrameTime());
        bg->oldPos = bg->pos;
        bg->pos = Vector3Add(bg->pos, direction);
        bg->box = UpdateBoundingBox(bg->origBox,bg->pos);
        bg->bodyBox = UpdateBoundingBox(bg->origBodyBox,bg->pos);
        bg->headBox = UpdateBoundingBox(bg->origHeadBox,bg->pos);
        if(Vector3Distance(bg->pos, bg->targetPos) < BG_TO_TARGET_POS_ACCEPT
            || HasTimerElapsed(&bg->t_walk_stuck,time(0)))
        {
            ResetTimer(&bg->t_walk_stuck);
            if(BgLineOfSightToMc(l,bg,index))
            {
                bg->state = BG_STATE_SHOOTING;
                bg->anim = bg->type==BG_TYPE_ARMY?ANIM_SHOOT:ANIM_YETI_JUMP;
                bg->curFrame = 0;
                bg->yaw = GetYawToTarget(bg->pos,mc->pos);
                if(bg->type==BG_TYPE_YETI && !IsSoundPlaying(bg->shootSound)){PlaySound(bg->shootSound);}//yeti shoot sound is handled here, army is in game.c in the updaetGame, bg animations section
                if(bg->type==BG_TYPE_YETI)//start jump
                {
                    bg->isJumping = true;
                    bg->yVelocity = YETI_JUMP_FORCE;
                    bg->jumpMove = Vector3Subtract(mc->pos, bg->pos);
                    bg->jumpMove.y = 0; //we only want the x and z of the player to jump toward
                    //we need to update the y here so isJumping isnt immedialty set to false by collision
                    Vector3 m = Vector3Normalize(bg->jumpMove);
                    m = Vector3Scale(m, bg->jumpSpeed * GetFrameTime());
                    bg->pos = Vector3Add(bg->pos, m);
                    bg->pos.y += bg->yVelocity * GetFrameTime();
                    bg->yVelocity -= GRAVITY;
                    bg->box = UpdateBoundingBox(bg->origBox,bg->pos);
                    bg->bodyBox = UpdateBoundingBox(bg->origBodyBox,bg->pos);
                    bg->headBox = UpdateBoundingBox(bg->origHeadBox,bg->pos);
                }
            }
            else //if no line of sight, he goes back to planning mode
            {
                bg->state = BG_STATE_PLANNING;
            }
        }
    }
    else if(bg->state == BG_STATE_SHOOTING)
    {
        if(bg->type==BG_TYPE_ARMY)
        {
            bg->yaw = GetYawToTarget(bg->pos,mc->pos);
        }
        else if(bg->type==BG_TYPE_YETI)
        {
            //printf("yeti is shooting ... \n");
            Vector3 m = Vector3Normalize(bg->jumpMove);
            m = Vector3Scale(m, bg->jumpSpeed * GetFrameTime());
            bg->pos = Vector3Add(bg->pos, m);
            bg->pos.y += bg->yVelocity * GetFrameTime();
            bg->yVelocity -= GRAVITY;
            if(bg->yVelocity < TERMINAL_Y_VEL) //if he fell to his death
            {
                bg->yVelocity = TERMINAL_Y_VEL;
                bg->dead = true;
            }
            bg->box = UpdateBoundingBox(bg->origBox,bg->pos);
            bg->bodyBox = UpdateBoundingBox(bg->origBodyBox,bg->pos);
            bg->headBox = UpdateBoundingBox(bg->origHeadBox,bg->pos);
        }
    }
}

void HandleBgArmyAnimEnd(Enemy *bg, Level *l, GameState *gs, int i)
{
    if(bg->state == BG_STATE_DYING)
    {
        bg->state = BG_STATE_DEAD;
        bg->dead = true;
    }
    else if(bg->state == BG_STATE_SHOOTING)
    {
        //todo: this might need to change location, possibly hard code to a frame above, to time getting shot at with the animation
        HandleBgShotPlayer(l,gs,i);
        //todo: this is good I think
        bg->state = BG_STATE_PLANNING;
        bg->anim = ANIM_WALKING;
    }
    else if(bg->state == BG_STATE_HIT)
    {
        bg->state = BG_STATE_PLANNING;
        bg->anim = ANIM_WALKING;
    }
}
void HandleBgYetiAnimEnd(Enemy *bg,Level *l, GameState *gs)
{
    if(bg->state == BG_STATE_DYING)
    {
        if(bg->drawColor.a == 0 || HasTimerElapsed(&bg->t_yeti_death_wait,time(0)))//keep in sync with draw bg in draw game
        {
            ResetTimer(&bg->t_yeti_death_wait);//not really needed but sure why not
            bg->state = BG_STATE_DEAD;
            bg->dead = true;
        }
    }
    else if(bg->state == BG_STATE_SHOOTING && !bg->isJumping)
    {
        bg->state = BG_STATE_PLANNING;
        bg->anim = ANIM_YETI_WALK;
    }
    else if(bg->state == BG_STATE_HIT)
    {
        bg->state = BG_STATE_PLANNING;
        bg->anim = ANIM_YETI_WALK;
    }
}

//does not use square root
bool IsWithinDistance(Vector3 a, Vector3 b, float maxDist)
{
    float dx = a.x - b.x;
    float dy = a.y - b.y;
    float dz = a.z - b.z;
    return (dx*dx + dy*dy + dz*dz) < (maxDist * maxDist);
}

static Plane NormalizePlane(Plane p) {
    float len = Vector3Length(p.normal);
    return (Plane){
        .normal = Vector3Scale(p.normal, 1.0f / len),
        .d = p.d / len
    };
}

Frustum ExtractFrustum(Matrix mat)
{
    Frustum f;

    // LEFT
    f.planes[0] = NormalizePlane((Plane){
        .normal = (Vector3){ mat.m3 + mat.m0, mat.m7 + mat.m4, mat.m11 + mat.m8 },
        .d = mat.m15 + mat.m12
    });

    // RIGHT
    f.planes[1] = NormalizePlane((Plane){
        .normal = (Vector3){ mat.m3 - mat.m0, mat.m7 - mat.m4, mat.m11 - mat.m8 },
        .d = mat.m15 - mat.m12
    });

    // BOTTOM
    f.planes[2] = NormalizePlane((Plane){
        .normal = (Vector3){ mat.m3 + mat.m1, mat.m7 + mat.m5, mat.m11 + mat.m9 },
        .d = mat.m15 + mat.m13
    });

    // TOP
    f.planes[3] = NormalizePlane((Plane){
        .normal = (Vector3){ mat.m3 - mat.m1, mat.m7 - mat.m5, mat.m11 - mat.m9 },
        .d = mat.m15 - mat.m13
    });

    // NEAR
    f.planes[4] = NormalizePlane((Plane){
        .normal = (Vector3){ mat.m3 + mat.m2, mat.m7 + mat.m6, mat.m11 + mat.m10 },
        .d = mat.m15 + mat.m14
    });

    // FAR
    f.planes[5] = NormalizePlane((Plane){
        .normal = (Vector3){ mat.m3 - mat.m2, mat.m7 - mat.m6, mat.m11 - mat.m10 },
        .d = mat.m15 - mat.m14
    });

    return f;
}

bool IsBoxInFrustum(BoundingBox box, Frustum frustum)
{
    for (int i = 0; i < 6; i++)
    {
        Plane plane = frustum.planes[i];

        // Find the corner of the AABB that is most *opposite* to the normal
        Vector3 positive = {
            (plane.normal.x >= 0) ? box.max.x : box.min.x,
            (plane.normal.y >= 0) ? box.max.y : box.min.y,
            (plane.normal.z >= 0) ? box.max.z : box.min.z
        };

        // If that corner is outside, the box is not visible
        float distance = Vector3DotProduct(plane.normal, positive) + plane.d;
        if (distance < 0)
            return false;
    }

    return true;
}

void DrawCustomFPS(int x, int y, Color color)
{
    int fps = GetFPS();
    char fpsText[16];
    sprintf(fpsText, "%d FPS", fps);
    DrawText(fpsText, x, y, 20, color);
}

void DrawHeart(Vector2 position, float size, Color color)
{
    float radius = size * 0.3f;

    Vector2 leftCircle = { position.x - radius, position.y };
    Vector2 rightCircle = { position.x + radius, position.y };
    Vector2 tip = { position.x, position.y + size * 0.8f };

    // Circles on top
    DrawCircleV(leftCircle, radius, color);
    DrawCircleV(rightCircle, radius, color);

    // Triangle forming the bottom of the heart
    Vector2 p1 = (Vector2){ position.x + size * 0.6f, position.y };
    Vector2 p2 = (Vector2){ position.x - size * 0.6f, position.y };
    Vector2 p3 = tip;

    // Clockwise order: p1 -> p2 -> p3
    DrawTriangle(p1, p2, p3, color);
}

//returns true if bg has line of sight to mc
bool BgLineOfSightToMc(Level *l, Enemy *bg, int index)
{
    // Step 1: Create a ray from the camera and detect collision (should always hit)
    Vector3 origin = bg->pos;
    Vector3 direction = Vector3Normalize(Vector3Subtract(l->mc.pos, bg->pos));
    Vector3 direction2 = Vector3Normalize(Vector3Subtract(l->mc.camera.position, bg->pos));
    Ray bulletRay = (Ray){ origin, direction };
    Ray bulletRay2 = (Ray){ origin, direction2 };
    RayCollision mcColl = GetRayCollisionBox(bulletRay, l->mc.box);
    //report if its not a hit because that is strange
    if(!mcColl.hit){printf("BgLineOfSightToMc, no hit on MC?\n");}
    // step 2: check if collision hits walls instead
    for(int i=0; i<l->objCount; i++)
    {
        if(l->obj[i].useHitBoxes)
        {
            for(int j=0; j<l->obj[i].hitBoxCount; j++)
            {
                RayCollision coll = GetRayCollisionBox(bulletRay, l->obj[i].hitBoxes[j]);
                RayCollision coll2 = GetRayCollisionBox(bulletRay2, l->obj[i].hitBoxes[j]);
                if(coll.hit && coll.distance < mcColl.distance && coll2.hit && coll2.distance < mcColl.distance)
                {
                    //printf("bg cant see mc because of BB\n");
                    return false;
                } 
            }
        }
        else
        {
            RayCollision coll = GetRayCollisionBox(bulletRay, l->obj[i].box);
            RayCollision coll2 = GetRayCollisionBox(bulletRay2, l->obj[i].box);
            if(coll.hit && coll.distance < mcColl.distance && coll2.hit && coll2.distance < mcColl.distance)
            {
                //printf("bg cant see mc because of wall\n");
                return false;
            }
        }
    }
    //printf("bg %d can see mc\n",index);
    return true;
}


