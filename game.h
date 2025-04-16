#ifndef GAME_H
#define GAME_H

#include "raylib.h"
#include "level.h"
#include "timer.h"

//constants
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600

//enums
typedef enum {
    SCREEN_MENU,
    SCREEN_PLAYING,
    SCREEN_OPTIONS,
    SCREEN_LEVEL_SELECT,
    SCREEN_IN_GAME_MENU,
    SCREEN_EXIT
} GameScreen;

typedef enum {
    DIFFICULTY_EASY,
    DIFFICULTY_NORMAL,
    DIFFICULTY_HARD
} Difficulty;

//structs
typedef struct {
    char name[128];
    char filename[128];
} MenuLevel;

typedef struct {
    GameScreen screen;
    Difficulty diff;
    bool invertY;
    bool showBoxes;
    bool drawTri;
    int menuSelection;
    int menuCount; // Play, Options, Exit
    int menuInGameSelection;
    int menuInGameCount; // Continue, Options, Level Select, Exit
    MenuLevel *levels;
    int levelCount;
    int levelSelection;
    Color fadeColor;
    Color deathFadeColor;
    Timer t_crouch_wait;
    Timer t_collDamage_wait;
    Timer t_endLevel_wait;
} GameState;

//functions
void UpdateGame(GameState *gs, Level *l);
void DrawGame(GameState *gs, Level *l);
void UpdateMainMenu(GameState *gs);
void UpdateOptionsMenu(GameState *gs, Level *l);
void UpdateLevelSelect(GameState *gs, Level *l);
void UpdateInGameMenu(GameState *gs, Level *l);

#endif // GAME_H