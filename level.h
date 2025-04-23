#ifndef LEVEL_H
#define LEVEL_H

#include "raylib.h"
#include "map_parser.h"
#include "timer.h"

//for deep copy of Model/Meshes and stuff in the model
#define MAX_MATERIAL_MAPS 12
#define MAX_MESH_VERTEX_BUFFERS 7
//constants for max list sizes
#define MAX_ENV_OBJECTS 1024
#define MAX_BAD_GUYS 64
#define MAX_ITEMS 64
#define MAX_HIT_BOXES 8
//constants for items and weapons and such
#define TOTAL_WEAPON_TYPES 2
// Constants for jumping and falling and such
#define GRAVITY 0.5f
#define JUMP_FORCE 8.0f
#define TERMINAL_Y_VEL -80.0f
#define DEAD_ZONE -100.0f
#define PAINFUL_Y_VEL -40.0f
//badguy state constants
#define BG_TO_MC_WAKE_UP_DIST 30
#define BG_TO_TARGET_POS_ACCEPT 2
#define YETI_JUMP_FORCE 20.0f
#define YETI_JUMP_FORCE 20.0f
#define YETI_IMPACT_RADIUS 15
//colors
#define BLOODRED (Color){ 138, 3, 3, 255 }


//enums
typedef enum {
    OBJECT_OTHER = 0,
    WORLDSPAWN_GROUND = 1,
    OBJECT_PLATFORM = 2,
} ObjectType;

typedef enum {
    ITEM_M1GRAND,
    ITEM_AMMO_M1GRAND,
    ITEM_SHOTGUN,
    ITEM_AMMO_SHOTGUN,
    ITEM_HEALTH
} ItemType;

typedef enum {
    WEAPON_M1GRAND = 0, //keep index as enum, very important
    WEAPON_SHOTGUN = 1,
} WeaponType;

typedef enum {
    ANIM_DEATH = 0,
    ANIM_HIT = 1,
    ANIM_SHOOT = 2,
    ANIM_WALKING = 3,
    //for the yeti
    ANIM_YETI_JUMP = 0,
    ANIM_YETI_ROAR = 1,
    ANIM_YETI_WALK = 2,
} BgAnimation;

typedef enum {
    BG_STATE_STILL,
    BG_STATE_PLANNING,
    BG_STATE_DEAD,
    BG_STATE_DYING,
    BG_STATE_HIT,
    BG_STATE_SHOOTING,
    BG_STATE_WALKING,
    //for the yeti
} BgState;

typedef enum {
    BG_TYPE_ARMY,
    BG_TYPE_YETI
} BgType;

//structs
typedef struct {
    BgType type;
    Model model;
    Color drawColor;
    BgState state;
    BgAnimation anim;
    ModelAnimation *anims;
    int animCount;
    int curFrame;
    BoundingBox box;
    BoundingBox origBox;
    BoundingBox headBox;
    BoundingBox origHeadBox;
    BoundingBox bodyBox;
    BoundingBox origBodyBox;
    Vector3 pos;
    bool dead;
    float health;
    Vector3 targetPos;
    float speed;
    Vector3 oldPos;
    float yaw;
    bool isFalling;
    float yOffset; //the model is not correct, so add this to y placement in collision detection with plats
    bool isShooter;
    float yVelocity;
    Vector3 jumpMove;
    float jumpSpeed;
    bool isJumping;
    Timer t_walk_stuck;
    Timer t_yeti_death_wait;
    Sound hitSound;
    Sound shootSound;
    Sound deathSound;
} Enemy;

typedef struct {
    ItemType type;
    Model model;
    BoundingBox box;
    Vector3 pos;
    bool isCollected;
} Item;

typedef struct {
    Model model;
    WeaponType type;
    char name[64];
    bool have;
    Vector3 gunPos;//for the 3d camera that is used
    float rot;
    float maxDist;
    float damage;
    int ammo;
    Sound shootSound;
} Weapon;

typedef struct {
    bool pointEntity;
    ObjectType type;
    Model model;
    BoundingBox box;
    Vector3 pos;
    float radius;
    bool useOrigin;
    Vector3 origin;
    bool useHitBoxes;
    int hitBoxCount;
    BoundingBox hitBoxes[MAX_HIT_BOXES];
    bool noCOll;
} EnvObject;

typedef struct {
    Model model;
    Vector3 pos;
    Vector3 oldPos;
    Vector3 startPos;
    BoundingBox box;
    BoundingBox originalBox;
    BoundingBox originalCrouchBox;
    Camera3D camera;
    float yaw;
    float pitch;
    float mouseSensitivity;
    float moveSpeed;
    float crouchSpeed;
    float yVelocity;
    bool isJumping;
    bool isFalling;
    bool isOnPlatform;
    bool isCrouching;
    float height;
    float crouchHeight;
    float width;
    float depth;
    float health;
    float maxHealth;
    bool hasAnyWeapon;
    int totalWeaponCount;
    int curWeaponIndex;
    Weapon weapons[TOTAL_WEAPON_TYPES];
    int score;
    int lives;
    Sound deathSound;
    Sound looseSound;
    Sound landSound;
} MainCharacter;

typedef struct {
    bool loaded;
    char name[128];
    char filename[128];
    MainCharacter mc;
    //lists
    int objCount;
    EnvObject *obj;
    int bgCount;
    Enemy *bg;
    int itemCount;
    Item *items;
    //unique lists for cleanup
    int uniqueTextures;
    Texture *uTextures;
    int uniqueModels;
    Model *uModels;
    int uniqueAnimations;
    ModelAnimation **uAnimations; // double pointer for this guy over here ... wtf man
    int *uNumAnimations;
    int uniqueSounds;
    Sound *uSounds;
} Level;

Level LoadLevel(const char *filename);
void UnloadLevel(Level * l);
BoundingBox UpdateBoundingBox(BoundingBox box, Vector3 pos);
void PrintVector3(char* mes, Vector3 v);
void PrintBoundingBox(char* mes, BoundingBox b);

#endif