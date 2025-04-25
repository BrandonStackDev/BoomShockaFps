#include "level.h"
#include "game.h"
#include "collision.h"
#include "functions.h"
#include "timer.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <math.h>   // for sinf, cosf
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void LoadGameStateSounds(GameState *gs)
{
    gs->selectSound=LoadSound("sounds/select.mp3");
    gs->enterSound=LoadSound("sounds/enter.mp3");
    gs->playSound=LoadSound("sounds/play.mp3");
    gs->music = LoadMusicStream("sounds/game_music.mp3");
}
void UnloadGameStateSounds(GameState *gs)
{
    UnloadSound(gs->selectSound);
    UnloadSound(gs->enterSound);
    UnloadSound(gs->playSound);
    UnloadMusicStream(gs->music);
}

void UpdateMainMenu(GameState *gs) {
    BeginDrawing();
        ClearBackground(BLACK);
        DrawText("BoomShockaFPS!", 100, 50, 40, WHITE);
        // Menu items
        const char *menuItems[] = { "Play", "Options", "Exit" };

        // Handle input
        if (IsKeyPressed(KEY_DOWN)) 
        {
            gs->menuSelection = (gs->menuSelection + 1) % gs->menuCount;
            PlaySound(gs->selectSound);
        }
        if (IsKeyPressed(KEY_UP)) 
        {
            gs->menuSelection = (gs->menuSelection - 1 + gs->menuCount) % gs->menuCount;
            PlaySound(gs->selectSound);
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) 
        {
            switch (gs->menuSelection) {
                case 0: gs->screen = SCREEN_LEVEL_SELECT; break;
                case 1: gs->screen  = SCREEN_OPTIONS; break;
                case 2: gs->screen  = SCREEN_EXIT; break;
            }
            PlaySound(gs->enterSound);
        }

        // Draw menu
        for (int i = 0; i < gs->menuCount; i++) {
            Color color = (i == gs->menuSelection) ? YELLOW : GRAY;
            int y = 150 + i * 50;
            DrawText(menuItems[i], 100, y, 30, color);
        }
    EndDrawing();
}


void UpdateOptionsMenu(GameState *gs, Level *l) 
{
    BeginDrawing();
        ClearBackground(DARKGRAY);
        DrawText("OPTIONS", 100, 50, 30, WHITE);

        // Difficulty toggle
        if (IsKeyPressed(KEY_D)) {
            gs->diff = (gs->diff + 1) % 3;
            PlaySound(gs->selectSound);
        }
        const char* difficultyText = (gs->diff == DIFFICULTY_EASY) ? "Easy" :
                                    (gs->diff == DIFFICULTY_NORMAL) ? "Normal" : "Hard";
        DrawText(TextFormat("Difficulty: %s (press D)", difficultyText), 100, 150, 20, WHITE);

        // Y Inversion toggle
        if (IsKeyPressed(KEY_Y)) {
            gs->invertY = !gs->invertY;
            PlaySound(gs->selectSound);
        }
        DrawText(TextFormat("Y Inversion: %s (press Y)", gs->invertY ? "On" : "Off"), 100, 200, 20, WHITE);

        // Q Quick Fire toggle
        if (IsKeyPressed(KEY_Q)) {
            gs->quickFire = !gs->quickFire;
            PlaySound(gs->selectSound);
        }
        DrawText(TextFormat("Quick Fire: %s (press Q)", gs->quickFire ? "On" : "Off"), 100, 250, 20, WHITE);

        // p toggles music
        if (IsKeyPressed(KEY_P)) {
            if(gs->playMusic) //turn it off
            {
                gs->playMusic = false;
                SetMusicVolume(gs->music, 0.0f);
            }
            else //turn it back on
            {
                gs->playMusic = true;
                SetMusicVolume(gs->music, 1.0f);
            }
            PlaySound(gs->selectSound);
        }
        DrawText(TextFormat("Play Music: %s (press P)", gs->playMusic ? "On" : "Off"), 100, 300, 20, WHITE);

        DrawText("Press ENTER to make selections and go back", 100, 400, 20, WHITE);
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if(l->loaded){gs->screen = SCREEN_IN_GAME_MENU;}
            else{gs->screen = SCREEN_MENU;}
            PlaySound(gs->enterSound);
        }
    EndDrawing();
}

void UpdateLevelSelect(GameState *gs, Level *l) {
    BeginDrawing();
        ClearBackground(BLACK);
        DrawText("Level Select", 100, 50, 40, WHITE);

        // Handle input
        if (IsKeyPressed(KEY_DOWN)) 
        {
            gs->levelSelection = (gs->levelSelection + 1) % gs->levelCount;
            PlaySound(gs->selectSound);
        }
        if (IsKeyPressed(KEY_UP)) 
        {
            gs->levelSelection = (gs->levelSelection - 1 + gs->levelCount) % gs->levelCount;
            PlaySound(gs->selectSound);
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) 
        {
            PlaySound(gs->playSound);
            if(l->loaded)
            {
                printf("Unloading level ...\n");
                gs->fadeColor.a = 0;
                gs->deathFadeColor.a = 0;
                UnloadLevel(l);
            }
            Level levy = LoadLevel(gs->levels[gs->levelSelection].filename);
            *l = levy;
            l->loaded = true;
            gs->screen = SCREEN_PLAYING;
        }
        // Draw menu
        for (int i = 0; i < gs->levelCount; i++) {
            Color color = (i == gs->levelSelection) ? YELLOW : GRAY;
            int y = 150 + i * 50;
            DrawText(gs->levels[i].name, 100, y, 30, color);
        }
    EndDrawing();
}

void UpdateInGameMenu(GameState *gs, Level *l)
{
    BeginDrawing();
        ClearBackground(BLACK);
        DrawText("PAUSE...", 100, 50, 40, WHITE);
        // Menu items
        const char *menuItems[] = { "Continue", "Options", "Level Select", "Exit" };

        // Handle input
        if (IsKeyPressed(KEY_DOWN)) 
        {
            gs->menuInGameSelection = (gs->menuInGameSelection + 1) % gs->menuInGameCount;
            PlaySound(gs->selectSound);
        }
        if (IsKeyPressed(KEY_UP)) 
        {
            gs->menuInGameSelection = (gs->menuInGameSelection - 1 + gs->menuInGameCount) % gs->menuInGameCount;
            PlaySound(gs->selectSound);
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) 
        {
            switch (gs->menuInGameSelection) {
                case 0: gs->screen = SCREEN_PLAYING; break;
                case 1: gs->screen  = SCREEN_OPTIONS; break;
                case 2: gs->screen  = SCREEN_LEVEL_SELECT; break;
                case 3: gs->screen  = SCREEN_EXIT; break;
            }
            PlaySound(gs->enterSound);
        }

        // Draw menu
        for (int i = 0; i < gs->menuInGameCount; i++) {
            Color color = (i == gs->menuInGameSelection) ? YELLOW : GRAY;
            int y = 150 + i * 50;
            DrawText(menuItems[i], 100, y, 30, color);
        }
    EndDrawing();
}

void UpdateGame(GameState *gs, Level *l)
{
    float dt = GetFrameTime();
    time_t currentTime = time(NULL);

    //reset gs timers - handle reset of basic timers
    if(HasTimerElapsed(&gs->t_crouch_wait,currentTime))
    {
        ResetTimer(&gs->t_crouch_wait);
    }
    if(HasTimerElapsed(&gs->t_collDamage_wait,currentTime))
    {
        ResetTimer(&gs->t_collDamage_wait);
    }

    // Mouse look
    Vector2 mouseDelta = GetMouseDelta();
    l->mc.yaw -= mouseDelta.x * l->mc.mouseSensitivity;
    l->mc.pitch += gs->invertY ? mouseDelta.y * l->mc.mouseSensitivity : (-1 * mouseDelta.y * l->mc.mouseSensitivity);

    // Clamp pitch to prevent flip
    if (l->mc.pitch > PI/2.0f) {l->mc.pitch = PI/2.0f;}
    if (l->mc.pitch < -PI/2.0f) {l->mc.pitch = -PI/2.0f;}

    // Convert yaw/pitch to forward vector
    Vector3 forward = {
        cosf(l->mc.pitch) * sinf(l->mc.yaw),
        sinf(l->mc.pitch),//this makes you take off into the sky
        cosf(l->mc.pitch) * cosf(l->mc.yaw)
    };
    Vector3 forwardMove = forward;
    forwardMove.y = 0.0f;
    forwardMove = Vector3Normalize(forwardMove);

    Vector3 right = {
        sinf(l->mc.yaw - PI/2.0f),
        0.0f,
        cosf(l->mc.yaw - PI/2.0f)
    };
    //update bg states and movement and such
    for(int i=0; i<l->bgCount; i++)
    {
        l->bg[i].oldPos = l->bg[i].pos;//store this here
        HandleBgState(l, &l->mc, &l->bg[i], i);
    }
    // Movement
    l->mc.oldPos = l->mc.pos;//store old pos
    Vector3 move = { 0 };
    if (IsKeyDown(KEY_W)) {move = Vector3Add(move, forwardMove);}
    if (IsKeyDown(KEY_S)) {move = Vector3Subtract(move, forwardMove);}
    if (IsKeyDown(KEY_A)) {move = Vector3Subtract(move, right);}
    if (IsKeyDown(KEY_D)) {move = Vector3Add(move, right);}
    if (IsKeyDown(KEY_SPACE) && !l->mc.isJumping && !l->mc.isFalling) 
    { 
        if(l->mc.isCrouching)
        {
            l->mc.isCrouching = false;
        }
        else
        {
            l->mc.isJumping = true;
            l->mc.yVelocity=JUMP_FORCE; 
        }
    }
    if (IsKeyDown(KEY_P)) {printf("position: %f %f %f\n", l->mc.pos.x,l->mc.pos.y,l->mc.pos.z);}
    if (IsKeyDown(KEY_B)) {gs->showBoxes = !gs->showBoxes;}
    if (IsKeyDown(KEY_T)) {gs->drawTri = !gs->drawTri;}
    if (IsKeyDown(KEY_LEFT_CONTROL) && !l->mc.isJumping && !gs->t_crouch_wait.wasStarted)
    {
        l->mc.isCrouching = !l->mc.isCrouching;
        StartTimer(&gs->t_crouch_wait);
    }
    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && l->mc.hasAnyWeapon && l->mc.weapons[l->mc.curWeaponIndex].ammo > 0)
    {
        if(!IsSoundPlaying(l->mc.weapons[l->mc.curWeaponIndex].shootSound)||gs->quickFire)
        {
            PlaySound(l->mc.weapons[l->mc.curWeaponIndex].shootSound);
            ShootRay(l);
            l->mc.weapons[l->mc.curWeaponIndex].ammo -= 1;
        }
    }
    if (IsKeyPressed(KEY_RIGHT) && l->mc.hasAnyWeapon) 
    {
        int startIndex = l->mc.curWeaponIndex;
        do {
            l->mc.curWeaponIndex = (l->mc.curWeaponIndex + 1) % l->mc.totalWeaponCount;
        } while (!l->mc.weapons[l->mc.curWeaponIndex].have && l->mc.curWeaponIndex != startIndex);
    }
    if (IsKeyPressed(KEY_LEFT) && l->mc.hasAnyWeapon) 
    {
        int startIndex = l->mc.curWeaponIndex;
        do {
            l->mc.curWeaponIndex = (l->mc.curWeaponIndex - 1 + l->mc.totalWeaponCount) % l->mc.totalWeaponCount;
        } while (!l->mc.weapons[l->mc.curWeaponIndex].have && l->mc.curWeaponIndex != startIndex);
    }
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) //pause
    {
        gs->screen = SCREEN_IN_GAME_MENU;
    }

    //handle movement
    move = Vector3Scale(Vector3Normalize(move), (l->mc.isCrouching?l->mc.crouchSpeed:l->mc.moveSpeed) * dt);
    l->mc.pos = Vector3Add(l->mc.pos, move);

    //handle jumping logic, parabula (or striaght down if falling)
    if(l->mc.isJumping || l->mc.isFalling)
    {
        l->mc.pos.y += l->mc.yVelocity * dt;
        l->mc.yVelocity -= GRAVITY;
        if(l->mc.yVelocity < TERMINAL_Y_VEL)
        {
            //printf("y vel=%f\n", yVelocity);
            l->mc.yVelocity = TERMINAL_Y_VEL;
        }
    }
    //update bounding box on mc
    l->mc.box = UpdateBoundingBox(l->mc.isCrouching?l->mc.originalCrouchBox:l->mc.originalBox,l->mc.pos);
    //collision detection section---------------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //env objects and mc, before we detect collisions, we need to set platform to false;
    l->mc.isOnPlatform = false;
    for(int i=0; i<l->objCount; i++)
    {
        if(l->obj[i].noCOll){continue;}
        // if(Vector3Distance(l->mc.pos, l->obj[i].pos) < l->obj[i].radius + 2.2f 
        //     || CheckCollisionBoxes(l->mc.box, l->obj[i].box))
        if(CheckCollisionBoxes(l->mc.box, l->obj[i].box))
        {
            //printf("HandleObjectCollision: i=%d\n",i);
            if(!l->obj[i].useHitBoxes){HandleObjectCollision(&l->mc,&l->obj[i]);}
            else{HandleHitBoxesCollision(&l->mc,&l->obj[i]);}
        }
    }
    //bg and platforms, AND mc and bg
    for(int i=0;i<l->bgCount;i++)
    {
        if(l->bg[i].dead || l->bg[i].state == BG_STATE_DYING){continue;}
        HandleBgPlatCollision(&l->bg[i],l);
        HandleMcAndBgCollision(&l->mc,&l->bg[i],gs);
        if(l->bg[i].isFalling)
        {
            l->bg[i].pos.y += l->bg[i].yVelocity * dt;
            l->bg[i].yVelocity -= GRAVITY;
        }
        if(DEAD_ZONE > l->bg[i].pos.y){l->bg[i].dead = true;}//falling death of bad guy
        l->bg[i].box = UpdateBoundingBox(l->bg[i].origBox,l->bg[i].pos);
        l->bg[i].bodyBox = UpdateBoundingBox(l->bg[i].origBodyBox,l->bg[i].pos);
        l->bg[i].headBox = UpdateBoundingBox(l->bg[i].origHeadBox,l->bg[i].pos);
    }
    //items
    for(int i=0; i<l->itemCount; i++)
    {
        if(l->items[i].isCollected){continue;}
        HandleItemCollision(&l->mc,&l->items[i]);
    }
    //end collision detection section-----------------------------------------------------------------
    //------------------------------------------------------------------------------------------------
    //update bounding box on mc again, 
    l->mc.box = UpdateBoundingBox(l->mc.isCrouching?l->mc.originalCrouchBox:l->mc.originalBox,l->mc.pos);
    //handle setting is falling
    if(!l->mc.isOnPlatform && !l->mc.isJumping)
    {
        //printf("falling...\n");
        l->mc.isFalling = true;
    }
    //-----------BADGUY ANIMS------------------------------------------------------------------------
    for(int i=0; i<l->bgCount; i++)
    {
        if(l->bg[i].state == BG_STATE_STILL || l->bg[i].state == BG_STATE_PLANNING){continue;}
        //get frustum
        Matrix view = MatrixLookAt(l->mc.camera.position, l->mc.camera.target, l->mc.camera.up);
        Matrix proj = MatrixPerspective(DEG2RAD * l->mc.camera.fovy, SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
        Matrix vp = MatrixMultiply(view, proj);
        Frustum frustum = ExtractFrustum(vp);
        if(IsWithinDistance(l->bg[i].pos,l->mc.pos,100)&&IsBoxInFrustum(l->bg[i].box, frustum))
        {
            UpdateModelAnimation(l->bg[i].model, l->bg[i].anims[l->bg[i].anim], l->bg[i].curFrame);
        }
        l->bg[i].curFrame++;
        if(l->bg[i].type==BG_TYPE_ARMY && l->bg[i].state == BG_STATE_DYING){l->bg[i].pos.y += dt;}//this anim sinks too much into the ground
        if (l->bg[i].curFrame >= l->bg[i].anims[l->bg[i].anim].frameCount) 
        {
            l->bg[i].curFrame = 0;
            if(l->bg[i].type == BG_TYPE_ARMY){HandleBgArmyAnimEnd(&l->bg[i],l,gs,i);}
            else if (l->bg[i].type == BG_TYPE_YETI){HandleBgYetiAnimEnd(&l->bg[i],l,gs);}
        }
        if(l->bg[i].type==BG_TYPE_ARMY && l->bg[i].anim==ANIM_SHOOT && l->bg[i].curFrame==25)// && !IsSoundPlaying(l->bg[i].shootSound))
        {
            //printf("bg army shot sound coming from %d %s\n",i,l->bg[i].isShooter?"shooter":"walker");
            PlaySound(l->bg[i].shootSound);
        }
    }
    //-----------END BADGUY ANIMS--------------------------------------------------------------------
    //handle death
    if(DEAD_ZONE > l->mc.pos.y || l->mc.health <= 0)
    {
        l->mc.health = l->mc.maxHealth;
        l->mc.lives--;
        l->mc.pos = l->mc.startPos;
        l->mc.yVelocity = 0;
        l->mc.isJumping = false;
        l->mc.isFalling = false;
        PlaySound(l->mc.deathSound);
    }
    //update camera before draw
    l->mc.camera.position = (Vector3){
        l->mc.pos.x, 
        l->mc.isCrouching ? l->mc.pos.y + l->mc.crouchHeight : l->mc.pos.y + l->mc.height, 
        l->mc.pos.z};
    l->mc.camera.target = Vector3Add(l->mc.camera.position, forward);
}

void DrawGame(GameState *gs, Level *l)
{
    BeginDrawing();
        ClearBackground(SKYBLUE);
        BeginMode3D(l->mc.camera);
            //get frustum
            Matrix view = MatrixLookAt(l->mc.camera.position, l->mc.camera.target, l->mc.camera.up);
            Matrix proj = MatrixPerspective(DEG2RAD * l->mc.camera.fovy, SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 100.0f);
            Matrix vp = MatrixMultiply(view, proj);
            Frustum frustum = ExtractFrustum(vp);

            //draw static props / env objects
            for (int i = 0; i < l->objCount; i++)
            {
                if(!IsWithinDistance(l->obj[i].pos,l->mc.pos,200)||!IsBoxInFrustum(l->obj[i].box, frustum)){continue;}
                if(gs->drawTri)
                {
                    if(IsModelIndexed(&l->obj[i].model)){DrawTrianglesIndexed(&l->obj[i].model,l->obj[i].useOrigin,l->obj[i].origin);}
                    else{DrawTriangles(&l->obj[i].model,l->obj[i].useOrigin,l->obj[i].origin);}
                }
                else {DrawModel(l->obj[i].model, l->obj[i].useOrigin?l->obj[i].origin:(Vector3){0} , 1.0f, WHITE);}
                if(gs->showBoxes){DrawBoundingBox(l->obj[i].box, YELLOW);}
                if(gs->showBoxes && l->obj[i].useHitBoxes)
                {
                    for(int j=0; j<l->obj[i].hitBoxCount; j++)
                    {
                        DrawBoundingBox(l->obj[i].hitBoxes[j], DARKBLUE);
                    }
                }
            }
            //draw bad guys
            int deadBgCount = 0;
            for (int i = 0; i < l->bgCount; i++)
            {
                if(l->bg[i].dead){deadBgCount++; continue;}
                if(!IsWithinDistance(l->bg[i].pos,l->mc.pos,100)||!IsBoxInFrustum(l->bg[i].box, frustum)){continue;}
                if(gs->drawTri)
                {
                    if(IsModelIndexed(&l->bg[i].model)){DrawTrianglesIndexed(&l->bg[i].model,true,l->bg[i].pos);}
                    else{DrawTriangles(&l->bg[i].model,true,l->bg[i].pos);}
                }
                else
                {
                    if(l->bg[i].state == BG_STATE_DYING && l->bg[i].drawColor.a != 0)//keep in sync with yeti anim end for dying
                    {
                        l->bg[i].drawColor.a -= 1;
                    }//I like this fade out
                    DrawModelEx(l->bg[i].model, l->bg[i].pos,(Vector3){0,1,0}, RAD2DEG*l->bg[i].yaw, (Vector3){1,1,1}, l->bg[i].drawColor);
                }
                if(gs->showBoxes)
                {
                    DrawBoundingBox(l->bg[i].box, VIOLET);
                    DrawBoundingBox(l->bg[i].bodyBox, PURPLE);
                    DrawBoundingBox(l->bg[i].headBox, RED);
                }
            }
            //draw items
            for (int i = 0; i < l->itemCount; i++)
            {
                if(l->items[i].isCollected){continue;}
                if(!IsWithinDistance(l->items[i].pos,l->mc.pos,150)||!IsBoxInFrustum(l->items[i].box, frustum)){continue;}
                if(gs->drawTri)
                {
                    if(IsModelIndexed(&l->items[i].model)){DrawTrianglesIndexed(&l->items[i].model,true,l->items[i].pos);}
                    else{DrawTriangles(&l->items[i].model,true,l->items[i].pos);}
                }
                else{DrawModel(l->items[i].model, l->items[i].pos, 1.0f, WHITE);}
                if(gs->showBoxes){DrawBoundingBox(l->items[i].box, PINK);}
            }
            //draw mc stuff
            if(gs->showBoxes){DrawBoundingBox(l->mc.box, BLUE);}
        EndMode3D();
        //special function to draw mc's weapon
        if(l->mc.hasAnyWeapon){DrawGunHeld(l->mc.weapons[l->mc.curWeaponIndex].model,l->mc.camera,l->mc.weapons[l->mc.curWeaponIndex].gunPos,l->mc.weapons[l->mc.curWeaponIndex].rot);}
        //2d stuff
        DrawCustomFPS(700, 10, BLOODRED);
        DrawHealthBar((Vector2){700,40},80,5,l->mc.health/l->mc.maxHealth);
        //draw life hearts
        for (int i = 0; i < l->mc.lives; i++) 
        {
            DrawHeart((Vector2){712 + i * 30, 60}, 20, RED); // or BLOODRED
        }
        //score
        DrawText(TextFormat("Score: %d", l->mc.score), 10, 10, 20, BLOODRED);
        //2d weapon stuff
        if(l->mc.hasAnyWeapon)
        {
            DrawCrosshair();
            DrawText(TextFormat("Ammo: %d", l->mc.weapons[l->mc.curWeaponIndex].ammo), 10, 30, 20, BLOODRED);
            DrawText(TextFormat("Gun: %s", l->mc.weapons[l->mc.curWeaponIndex].name), 10, 50, 20, BLOODRED);
        }
        //handle end level sequences
        if(deadBgCount >= l->bgCount) //for debugging this || true)
        {
            DrawText(TextFormat("Level Cleared!"), (SCREEN_WIDTH/2.0f)-200, SCREEN_HEIGHT/2.0f, 50, BLOODRED);
            if(!gs->t_endLevel_wait.wasStarted)
            {
                StartTimer(&gs->t_endLevel_wait);
            }
            else if(HasTimerElapsed(&gs->t_endLevel_wait, time(0)))
            {
                gs->screen=SCREEN_MENU;
                ResetTimer(&gs->t_endLevel_wait);
            }
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, gs->fadeColor);
            if(gs->fadeColor.a < 255){gs->fadeColor.a++;}
        }
        else if(l->mc.lives <=0)//give benifit to player, if they won, even if they loose all lives at the last second, they still win
        {
            DrawText(TextFormat("DEATH!!!"), (SCREEN_WIDTH/2.0f)-127, SCREEN_HEIGHT/2.0f, 50, BLOODRED);
            if(!gs->t_endLevel_wait.wasStarted)
            {
                StartTimer(&gs->t_endLevel_wait);
                PlaySound(l->mc.looseSound);
            }
            else if(HasTimerElapsed(&gs->t_endLevel_wait, time(0)))
            {
                gs->screen=SCREEN_MENU;
                ResetTimer(&gs->t_endLevel_wait);
            }
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, gs->deathFadeColor);
            if(gs->deathFadeColor.a < 255){gs->deathFadeColor.a++;}
        }
    EndDrawing();
}