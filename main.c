#include "level.h"
#include "game.h"
#include "timer.h"
#include "raylib.h"
#include "raymath.h"
#include "rlgl.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <libgen.h>
#include <unistd.h>
#endif


// logical or ||
static GameState gs;
static Level l;

void SetWorkingDirectoryToAppResources()
{
#ifdef __APPLE__
    char path[1024];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0)
    {
        char *dir = dirname(path);
        chdir(dir); // Move to Contents/MacOS
        chdir("../Resources"); // Move to Resources folder
    }
#endif
}

void GameLoop(void) 
{
    switch (gs.screen) 
    {
        case SCREEN_MENU:
            if(l.loaded)
            {
                printf("SCREEN_MENU - Unloading level ...\n");
                gs.fadeColor.a = 0;
                gs.deathFadeColor.a = 0;
                UnloadLevel(&l);
            }
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
            if(l.loaded){UnloadLevel(&l);}
            MemFree(gs.levels);
            CloseWindow();
            #ifdef PLATFORM_WEB
                emscripten_force_exit(0);
            #endif
            exit(0);
            break;
    }
}

int main(void)
{
    //this one is for Mac .app folder, to find asset folders
    SetWorkingDirectoryToAppResources();
    //random behavior please
    srand((unsigned int)time(NULL));//this seeds with time for all other random calls
    //init window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Boom Shocka!");
    //audio
    InitAudioDevice();  // IMPORTANT: Must initialize audio!
    // Hide mouse and capture it
    DisableCursor();
    //set target FPS
    SetTargetFPS(60);

    //create level and game state
    l = (Level){0};
    l.loaded = false;
    gs = (GameState){0};
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

    #ifdef PLATFORM_WEB
        emscripten_set_main_loop(GameLoop, 0, 1);
    #else
        while (!WindowShouldClose())
        {
            GameLoop();
        }
    #endif
    
    if(l.loaded){UnloadLevel(&l);}
    MemFree(gs.levels);
    CloseWindow();

    #ifdef PLATFORM_WEB
        emscripten_force_exit(0); // not sure if this is needed here anymore? but just incase.
    #endif

    return 0;
}
