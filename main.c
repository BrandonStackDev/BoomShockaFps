#include "level.h"
#include "game.h"
#include "timer.h"
#include "collision.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// logical or ||

int main(void)
{
    //random behavior please
    srand((unsigned int)time(NULL));//this seeds with time for all other random calls
    //init window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Boom Shocka!");
    // Hide mouse and capture it
    DisableCursor();
    SetTargetFPS(60);
    
    Level l = {0};
    l.loaded = false;
    GameState gs = {0};
    //screen and menu setup
    gs.screen = SCREEN_MENU;
    gs.menuCount = 3;
    gs.menuSelection = 0;
    gs.menuInGameCount = 4;
    gs.menuInGameSelection = 0;
    //fade color
    gs.fadeColor = (Color){ 0, 0, 0, 0 };
    gs.deathFadeColor = BLOODRED;
    gs.deathFadeColor.a = 0;
    //setup levels
    gs.levelCount = 2;
    gs.levelSelection=  0;
    gs.levels = malloc(sizeof(MenuLevel) * gs.levelCount);
    gs.levels[0] = (MenuLevel){"Open Space","maps/test001.map"};
    gs.levels[1] = (MenuLevel){"The Maze","maps/test002.map"};
    //timers
    gs.t_crouch_wait = CreateTimer(0.0011f);
    gs.t_crouch_wait.virgin = false;
    gs.t_collDamage_wait = CreateTimer(0.666f);
    gs.t_collDamage_wait.virgin = false;
    gs.t_endLevel_wait = CreateTimer(5.0f);
    gs.t_endLevel_wait.virgin = false;

    while (!WindowShouldClose())
    {
        switch (gs.screen) {
            case SCREEN_MENU:
                UpdateMainMenu(&gs);
                break;
            case SCREEN_OPTIONS:
                UpdateOptionsMenu(&gs,&l);
                break;
            case SCREEN_LEVEL_SELECT:
                UpdateLevelSelect(&gs,&l);
                break;
            case SCREEN_PLAYING:
                UpdateGame(&gs,&l);
                DrawGame(&gs,&l);
                break;
            case SCREEN_IN_GAME_MENU:
                UpdateInGameMenu(&gs,&l);
                break;
            case SCREEN_EXIT:
                CloseWindow();
                exit(0);
                //clean up here also
                break;
        }
    }
    
    if(l.loaded){UnloadLevel(&l);}
    CloseWindow();
    
    return 0;
}
