#include "level.h"
#include "map_parser.h"
#include "timer.h"
#include "raylib.h"
#include "raymath.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static float RadiusFromModelAndCenter(Vector3 center, Model m)
{
    float maxRadius = 0;
    for(int i = 0; i < m.meshes[0].vertexCount; i+=3)
    {
        Vector3 point = {
            m.meshes[0].vertices[i],
            m.meshes[0].vertices[i+1],
            m.meshes[0].vertices[i+2]
        };
        if (Vector3Distance(center, point) > maxRadius){maxRadius = Vector3Distance(center, point);}
    }
    return maxRadius;
}

static Vector3 PositionFromBox(BoundingBox b)
{
    return (Vector3){
        (b.min.x + b.max.x) / 2.0f,
        (b.min.y + b.max.y) / 2.0f,
        (b.min.z + b.max.z) / 2.0f
    };
}

void PrintVector3(char* mes, Vector3 v)
{
    printf("%s, x=%f, y=%f, z=%f\n", mes, v.x,v.y,v.z);
}

void PrintBoundingBox(char* mes, BoundingBox b)
{
    PrintVector3(mes,b.min);
    PrintVector3(mes,b.max);
}

BoundingBox UpdateBoundingBox(BoundingBox box, Vector3 pos)
{
    BoundingBox newBox = {
        (Vector3){ box.min.x + pos.x, box.min.y + pos.y, box.min.z + pos.z },
        (Vector3){ box.max.x + pos.x, box.max.y + pos.y, box.max.z + pos.z }
    };
    return newBox;
}

static Mesh DeepCopyMesh(Mesh src)
{
    Mesh dst = src;

    // Deep copy mandatory buffers
    dst.vertices    = MemAlloc(sizeof(float) * src.vertexCount * 3);
    dst.normals     = MemAlloc(sizeof(float) * src.vertexCount * 3);
    dst.texcoords   = MemAlloc(sizeof(float) * src.vertexCount * 2);
    dst.indices     = MemAlloc(sizeof(unsigned short) * src.triangleCount * 3);

    memcpy(dst.vertices, src.vertices, sizeof(float) * src.vertexCount * 3);
    memcpy(dst.normals, src.normals, sizeof(float) * src.vertexCount * 3);
    memcpy(dst.texcoords, src.texcoords, sizeof(float) * src.vertexCount * 2);
    memcpy(dst.indices, src.indices, sizeof(unsigned short) * src.triangleCount * 3);

    // Optional buffers
    dst.tangents = src.tangents ? MemAlloc(sizeof(float) * src.vertexCount * 4) : NULL;
    if (src.tangents) memcpy(dst.tangents, src.tangents, sizeof(float) * src.vertexCount * 4);

    dst.colors = src.colors ? MemAlloc(sizeof(unsigned char) * src.vertexCount * 4) : NULL;
    if (src.colors) memcpy(dst.colors, src.colors, sizeof(unsigned char) * src.vertexCount * 4);

    dst.texcoords2 = src.texcoords2 ? MemAlloc(sizeof(float) * src.vertexCount * 2) : NULL;
    if (src.texcoords2) memcpy(dst.texcoords2, src.texcoords2, sizeof(float) * src.vertexCount * 2);

    dst.boneIds = src.boneIds ? MemAlloc(sizeof(unsigned char) * src.vertexCount * 4) : NULL;
    if (src.boneIds) memcpy(dst.boneIds, src.boneIds, sizeof(unsigned char) * src.vertexCount * 4);

    dst.boneWeights = src.boneWeights ? MemAlloc(sizeof(float) * src.vertexCount * 4) : NULL;
    if (src.boneWeights) memcpy(dst.boneWeights, src.boneWeights, sizeof(float) * src.vertexCount * 4);

    dst.animVertices = src.animVertices ? MemAlloc(sizeof(float) * src.vertexCount * 3) : NULL;
    if (src.animVertices) memcpy(dst.animVertices, src.animVertices, sizeof(float) * src.vertexCount * 3);

    dst.animNormals = src.animNormals ? MemAlloc(sizeof(float) * src.vertexCount * 3) : NULL;
    if (src.animNormals) memcpy(dst.animNormals, src.animNormals, sizeof(float) * src.vertexCount * 3);

    // Clear GPU IDs to trigger re-upload
    dst.vaoId = 0;
    for (int i = 0; i < MAX_MESH_VERTEX_BUFFERS; i++) dst.vboId[i] = 0;

    return dst;
}

Model DeepCopyModel(Model src)
{
    Model dst = src;

    // Deep copy meshes
    dst.meshes = MemAlloc(sizeof(Mesh) * src.meshCount);
    for (int i = 0; i < src.meshCount; i++)
    {
        dst.meshes[i] = DeepCopyMesh(src.meshes[i]);
        UploadMesh(&dst.meshes[i], true);
    }

    // Deep copy mesh-material mapping
    dst.meshMaterial = MemAlloc(sizeof(int) * src.meshCount);
    memcpy(dst.meshMaterial, src.meshMaterial, sizeof(int) * src.meshCount);

    // Deep copy materials
    dst.materials = MemAlloc(sizeof(Material) * src.materialCount);
    for (int i = 0; i < src.materialCount; i++)
    {
        dst.materials[i] = src.materials[i];
        dst.materials[i].maps = MemAlloc(sizeof(MaterialMap) * MAX_MATERIAL_MAPS);
        memcpy(dst.materials[i].maps, src.materials[i].maps, sizeof(MaterialMap) * MAX_MATERIAL_MAPS);
    }

    // Deep copy bones
    dst.bones = src.boneCount > 0 ? MemAlloc(sizeof(BoneInfo) * src.boneCount) : NULL;
    for (int i = 0; i < src.boneCount; i++)
    {
        dst.bones[i] = src.bones[i];
    }

    // Deep copy bind pose
    dst.bindPose = src.boneCount > 0 ? MemAlloc(sizeof(Transform) * src.boneCount) : NULL;
    for (int i = 0; i < src.boneCount; i++)
    {
        dst.bindPose[i] = src.bindPose[i];
    }

    return dst;
}

bool IsPowerOfTwo(int x) {
    return (x & (x - 1)) == 0;
}

bool IsTexturePOT(Texture2D tex) {
    return IsPowerOfTwo(tex.width) && IsPowerOfTwo(tex.height);
}

Texture GetText(const char *filename)
{
    Image image = LoadImage(filename);
    Texture2D texture = LoadTextureFromImage(image);
    printf("Texture: %s - %dx%d\n", filename, texture.width, texture.height);
    #ifdef PLATFORM_WEB
        if (IsTexturePOT(texture)) 
        {
            GenTextureMipmaps(&texture); // <-- allow it now that we're POT
            SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
            SetTextureWrap(texture, TEXTURE_WRAP_REPEAT);
        }
        else 
        {
            SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
            SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
        }
    #else
        GenTextureMipmaps(&texture);  // <-- this generates mipmaps
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR); // use a better filter
        //SetTextureFilter(texture, TEXTURE_FILTER_ANISOTROPIC_8X); //4x, 8x, 16x, depending on GPU, alternate if we need/want
    #endif 
    UnloadImage(image);
    return texture;
}

Level LoadLevel(const char *filename)
{
    printf("new level\n");
    Level level = {0};
    //load textures
    printf("textures\n");
    Texture2D wallTexture = GetText("textures/brick1.png"); // Load texture
    Texture2D platTexture = GetText("textures/wood1.png"); // Load texture
    Texture2D groundTexture = GetText("textures/grass1.png"); // Load texture
    Texture2D roofTexture = GetText("textures/roof.png"); // Load texture
    //store for cleanup
    printf("textures malloc\n");
    level.uniqueTextures = 4;
    level.uTextures = MemAlloc(sizeof(Texture) * level.uniqueTextures);
    level.uTextures[0] = wallTexture;
    level.uTextures[1] = platTexture;
    level.uTextures[2] = groundTexture;
    level.uTextures[3] = roofTexture;
    //models
    printf("models\n");
    Model treeModel = LoadModel("models/tree.glb");
    Model treeBgModel = LoadModel("models/tree_bg.glb");
    Model armyModel = LoadModel("models/soldier_4_anim.glb");
    Model m1Model = LoadModel("models/m1grand.glb");
    Model m1AmmoModel = LoadModel("models/ammo_m1grand.glb");
    Model sgModel = LoadModel("models/shotgun.glb");
    Model sgAmmoModel = LoadModel("models/ammo_shotgun.glb");
    Model healthModel = LoadModel("models/health_pack.glb");
    Model yetiModel = LoadModel("models/yeti_anim_2.glb");
    //store for cleanup
    printf("models malloc\n");
    level.uniqueModels = 9;
    level.uModels = MemAlloc(sizeof(Model) * level.uniqueModels);
    level.uModels[0] = treeModel;
    level.uModels[1] = treeBgModel;
    level.uModels[2] = armyModel;
    level.uModels[3] = m1Model;
    level.uModels[4] = m1AmmoModel;
    level.uModels[5] = sgModel;
    level.uModels[6] = sgAmmoModel;
    level.uModels[7] = healthModel;
    level.uModels[8] = yetiModel;

    printf("anims\n");
    //animations
    int armyAnimCount = 0;
    ModelAnimation *armyAnimations = LoadModelAnimations("models/soldier_4_anim.glb", &armyAnimCount);
    int yetiAnimCount = 0;
    ModelAnimation *yetiAnimations = LoadModelAnimations("models/yeti_anim_2.glb", &yetiAnimCount);
    //store for cleanup
    printf("anims malloc\n");
    level.uniqueAnimations = 2;
    level.uAnimations = MemAlloc(sizeof(ModelAnimation*) * level.uniqueAnimations);
    level.uNumAnimations = MemAlloc(sizeof(int) * level.uniqueAnimations);
    level.uAnimations[0] = armyAnimations;
    level.uNumAnimations[0] = armyAnimCount;
    level.uAnimations[1] =yetiAnimations;
    level.uNumAnimations[1] = yetiAnimCount;
    
    //sounds
    printf("sounds\n");
    Sound deathSound = LoadSound("sounds/scream.mp3");
    Sound looseSound = LoadSound("sounds/mc_death.mp3");
    Sound yetiRoar = LoadSound("sounds/yeti_roar.mp3");
    Sound bgHit = LoadSound("sounds/bg_hit.mp3");
    Sound bgDeath = LoadSound("sounds/bg_death.mp3");
    Sound bgShoot = LoadSound("sounds/bg_shoot.mp3");
    Sound shotgunSound = LoadSound("sounds/shotgun.mp3");
    Sound m1grandSound = LoadSound("sounds/m1grand.mp3");
    Sound landSound = LoadSound("sounds/land.mp3");
    printf("sounds malloc\n");
    level.uniqueSounds = 9;
    level.uSounds = MemAlloc(sizeof(Sound) * level.uniqueSounds);
    level.uSounds[0] = deathSound;
    level.uSounds[1] = looseSound;
    level.uSounds[2] = yetiRoar;
    level.uSounds[3] = bgHit;
    level.uSounds[4] = bgDeath;
    level.uSounds[5] = bgShoot;
    level.uSounds[6] = shotgunSound;
    level.uSounds[7] = m1grandSound;
    level.uSounds[8] = landSound;

    printf("mc creation\n");
    //create main character
    MainCharacter mc = {0};
    //defaults
    mc.camera = (Camera3D){ 0 };
    mc.camera.target = (Vector3){ 0, 0, 0 };
    mc.camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    mc.camera.fovy = 45.0f;
    mc.camera.projection = CAMERA_PERSPECTIVE;
    mc.yaw = 0.0f;
    mc.pitch = 0.0f;
    mc.mouseSensitivity = 0.003f;
    mc.moveSpeed = 10.0f;
    mc.crouchSpeed = 5.0f;
    mc.yVelocity = 0.0f;
    mc.isFalling = false;
    mc.isJumping = false;
    mc.isOnPlatform = false;
    mc.isCrouching = false;
    mc.height = 1.8f;
    mc.crouchHeight = 0.45f;
    mc.width = 1.2f;
    mc.depth = 1.2f;
    mc.health = 100;
    mc.maxHealth = 100;
    //bounding boxes
    mc.originalBox = (BoundingBox) {
        (Vector3){-1.0f*mc.width/2.0f, 0, -1.0f*mc.depth/2.0f},
        (Vector3){mc.width/2.0f,mc.height, mc.depth/2.0f}
    };
    mc.originalCrouchBox = (BoundingBox) {
        (Vector3){-1.0f*mc.width/2.0f, 0, -1.0f*mc.depth/2.0f},
        (Vector3){mc.width/2.0f,mc.crouchHeight, mc.depth/2.0f}
    };
    mc.box = mc.originalBox;
    //weapons
    mc.totalWeaponCount = TOTAL_WEAPON_TYPES;
    mc.hasAnyWeapon = false;
    mc.curWeaponIndex = 0;
    //m1grand
    mc.weapons[WEAPON_M1GRAND].type = WEAPON_M1GRAND;//keep index as enum, very important
    strcpy(mc.weapons[WEAPON_M1GRAND].name, "m1grand");
    mc.weapons[WEAPON_M1GRAND].model = m1Model;
    mc.weapons[WEAPON_M1GRAND].gunPos = (Vector3) { -0.3f, -0.4f, 0.8f };
    mc.weapons[WEAPON_M1GRAND].rot = 90;
    mc.weapons[WEAPON_M1GRAND].maxDist = 35;
    mc.weapons[WEAPON_M1GRAND].damage = 15;
    mc.weapons[WEAPON_M1GRAND].ammo = 25;
    mc.weapons[WEAPON_M1GRAND].shootSound = m1grandSound;
    //shotgun
    mc.weapons[WEAPON_SHOTGUN].type = WEAPON_SHOTGUN;//keep index as enum, very important
    strcpy(mc.weapons[WEAPON_SHOTGUN].name, "shotgun");
    mc.weapons[WEAPON_SHOTGUN].model = sgModel;
    mc.weapons[WEAPON_SHOTGUN].gunPos = (Vector3) { -0.3f, -0.4f, 0.8f };
    mc.weapons[WEAPON_SHOTGUN].rot = 90;
    mc.weapons[WEAPON_SHOTGUN].maxDist = 16;
    mc.weapons[WEAPON_SHOTGUN].damage = 30;
    mc.weapons[WEAPON_SHOTGUN].ammo = 15;
    mc.weapons[WEAPON_SHOTGUN].shootSound = shotgunSound;
    //mc sounds
    mc.deathSound = deathSound;
    mc.looseSound = looseSound;
    mc.landSound = landSound;
    //set important stuff
    mc.score=0;//starts fresh every level load
    mc.lives=3;//always starts at 3
    //set the mc in the level
    level.mc = mc;

    printf("-------------- load map file ------------\n");
    printf("test\n");
    // Load the models from .map
    int entityCount = 0;
    Entity *entities = LoadMapFile(filename, &entityCount);
    printf("Entity Count: %d\n", entityCount);
    
    //define the lists of things
    EnvObject objects[MAX_ENV_OBJECTS];
    int objCount = 0;
    Enemy badguys[MAX_BAD_GUYS];
    int bgCount = 0;
    Item items[MAX_ITEMS];
    int itemCount = 0;

    for (int i = 0; i < entityCount; i++)
    {
        if(strcmp(entities[i].className,"worldspawn")==0)
        {
            memset(&objects[objCount], 0, sizeof(EnvObject));
            objects[objCount].pointEntity = false;
            objects[objCount].type = WORLDSPAWN_GROUND;
            objects[objCount].model = entities[i].model;
            objects[objCount].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = groundTexture;
            objCount++;
        }
        else if(strcmp(entities[i].className,"func_wall")==0)
        {
            memset(&objects[objCount], 0, sizeof(EnvObject));
            objects[objCount].pointEntity = false;
            objects[objCount].model = entities[i].model;
            objects[objCount].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = wallTexture;
            objCount++;
        }
        else if(strcmp(entities[i].className,"func_plat")==0)
        {
            memset(&objects[objCount], 0, sizeof(EnvObject));
            objects[objCount].pointEntity = false;
            objects[objCount].type = OBJECT_PLATFORM;
            objects[objCount].model = entities[i].model;
            objects[objCount].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = platTexture;
            objCount++;
        }
        else if(strcmp(entities[i].className,"testplayerstart")==0)
        {
            //mc, this is well controlled so no memset, would erase our already set stuff any way
            level.mc.pos = entities[i].origin;
            level.mc.oldPos = entities[i].origin;
            level.mc.startPos = entities[i].origin;
            level.mc.camera.position = entities[i].origin;
            level.mc.camera.position.y = level.mc.height;
            level.mc.box = UpdateBoundingBox(level.mc.originalBox,entities[i].origin);
            level.mc.camera.target = Vector3Add(level.mc.camera.position, (Vector3){ 0.0f, 0.0f, 1.0f });
        }
        else if(strcmp(entities[i].className,"tree")==0)
        {
            memset(&objects[objCount], 0, sizeof(EnvObject));
            objects[objCount].pointEntity = true;
            objects[objCount].model = treeModel;
            objects[objCount].useOrigin = true;
            objects[objCount].origin = entities[i].origin;
            objects[objCount].origin.y-=0.5;//tree model floats a bit
            objects[objCount].useHitBoxes = true;
            objects[objCount].hitBoxCount = 2;
            BoundingBox box0 = {(Vector3){-1, 0, -1},(Vector3){1,5,1}};
            BoundingBox box1 = {(Vector3){-4, 5, -4},(Vector3){4,10,4}};
            objects[objCount].hitBoxes[0] = UpdateBoundingBox(box0,entities[i].origin);
            objects[objCount].hitBoxes[1] = UpdateBoundingBox(box1,entities[i].origin);
            objCount++;
        }
        else if(strcmp(entities[i].className,"tree_bg")==0)
        {
            memset(&objects[objCount], 0, sizeof(EnvObject));
            objects[objCount].pointEntity = true;
            objects[objCount].model = treeBgModel;
            objects[objCount].useOrigin = true;
            objects[objCount].origin = entities[i].origin;
            objects[objCount].origin.y-=0.35;//tree_bg model floats a bit
            objects[objCount].useHitBoxes = true;
            objects[objCount].hitBoxCount = 2;
            BoundingBox box0 = {(Vector3){-1, 0, -1},(Vector3){1,5,1}};
            BoundingBox box1 = {(Vector3){-4, 5, -4},(Vector3){4,10,4}};
            objects[objCount].hitBoxes[0] = UpdateBoundingBox(box0,entities[i].origin);
            objects[objCount].hitBoxes[1] = UpdateBoundingBox(box1,entities[i].origin);
            objCount++;
        }
        else if(strcmp(entities[i].className,"monster_army")==0)
        {
            memset(&badguys[bgCount], 0, sizeof(Enemy));
            //for now we open the army man more than once and just do that
            //Model armyModel = LoadModel("models/soldier_anim.glb");
            badguys[bgCount].type = BG_TYPE_ARMY;
            badguys[bgCount].drawColor = WHITE; //always white
            #ifdef MEMORY_SAFE_MODE
                badguys[bgCount].model = LoadModel("models/soldier_4_anim.glb");
            #else
                badguys[bgCount].model = DeepCopyModel(armyModel);
            #endif
            if(entities[i].hasSubType)
            {
                if(strcmp(entities[i].subType,"shooter")==0)
                {
                   badguys[bgCount].isShooter = true; 
                }
            }
            badguys[bgCount].anims = armyAnimations;
            badguys[bgCount].animCount = armyAnimCount;
            badguys[bgCount].pos = entities[i].origin;
            badguys[bgCount].pos.y-=0.1f;//they float, origin problem
            badguys[bgCount].yOffset=0.3f;//the model itself is defined below where it needs to be, offset to correct
            badguys[bgCount].health = 50;
            badguys[bgCount].state = BG_STATE_STILL;
            badguys[bgCount].anim = ANIM_WALKING;
            badguys[bgCount].speed = 4;
            BoundingBox box0 = {(Vector3){-0.25f, 0, -0.25f},(Vector3){0.25f,1.4f,0.25f}};//body
            BoundingBox box1 = {(Vector3){-0.13f, 1.4f, -0.13f},(Vector3){0.13f,1.7f,0.13f}};//head
            badguys[bgCount].origBodyBox = box0;
            badguys[bgCount].origHeadBox = box1;
            badguys[bgCount].t_walk_stuck = CreateTimer(8);//walk no more than 8 seconds
            badguys[bgCount].t_yeti_death_wait = CreateTimer(5);//specifically for yetis, but a death timer to help guide fade effects
            badguys[bgCount].hitSound = bgHit;
            badguys[bgCount].shootSound = bgShoot;
            badguys[bgCount].deathSound = bgDeath;
            bgCount++;
        }
        else if(strcmp(entities[i].className,"monster_ogre")==0)//yeti
        {
            memset(&badguys[bgCount], 0, sizeof(Enemy));
            badguys[bgCount].type = BG_TYPE_YETI;
            badguys[bgCount].drawColor = WHITE;
            #ifdef MEMORY_SAFE_MODE
                badguys[bgCount].model = LoadModel("models/yeti_anim_2.glb");
            #else
                badguys[bgCount].model = DeepCopyModel(yetiModel);
            #endif
            badguys[bgCount].anims = yetiAnimations;
            badguys[bgCount].animCount = yetiAnimCount;
            badguys[bgCount].pos = entities[i].origin;
            badguys[bgCount].pos.y+=0.0f;
            badguys[bgCount].yOffset=0.0f;
            badguys[bgCount].health = 200;
            badguys[bgCount].state = BG_STATE_STILL;
            badguys[bgCount].anim = ANIM_YETI_WALK;
            badguys[bgCount].speed = 4;
            badguys[bgCount].jumpSpeed = 8;
            BoundingBox box0 = {(Vector3){-1.8f, 0.3f, -1.8f},(Vector3){1.8f,5,1.8f}};//body
            BoundingBox box1 = {(Vector3){-1, 5, -1},(Vector3){1,6,1}};//head
            badguys[bgCount].origBodyBox = box0;
            badguys[bgCount].origHeadBox = box1;
            badguys[bgCount].t_walk_stuck = CreateTimer(8);//walk no more than 8 seconds
            badguys[bgCount].t_yeti_death_wait = CreateTimer(5);//specifically for yetis, but a death timer to help guide fade effects
            badguys[bgCount].hitSound = yetiRoar;
            badguys[bgCount].shootSound = yetiRoar;
            badguys[bgCount].deathSound = yetiRoar;
            bgCount++;
        }
        else if(strcmp(entities[i].className,"weapon_m1grand")==0)
        {
            memset(&items[itemCount], 0, sizeof(Item));
            items[itemCount].type = ITEM_M1GRAND;
            items[itemCount].model = m1Model;
            items[itemCount].pos = entities[i].origin;
            items[itemCount].pos.y += 0.8f; // sinks into ground without this
            itemCount++;
        }
        else if(strcmp(entities[i].className,"item_health")==0)
        {
            memset(&items[itemCount], 0, sizeof(Item));
            items[itemCount].type = ITEM_HEALTH;
            items[itemCount].model = healthModel;
            items[itemCount].pos = entities[i].origin;
            //items[itemCount].pos.y += 0.8f; // sinks into ground without this
            itemCount++;
        }
        else if(strcmp(entities[i].className,"ammo_m1grand")==0)
        {
            memset(&items[itemCount], 0, sizeof(Item));
            items[itemCount].type = ITEM_AMMO_M1GRAND;
            items[itemCount].model = m1AmmoModel;
            items[itemCount].pos = entities[i].origin;
            //items[itemCount].pos.y += 0.8f; // sinks into ground without this
            itemCount++;
        }
        else if(strcmp(entities[i].className,"weapon_shotgun")==0)
        {
            memset(&items[itemCount], 0, sizeof(Item));
            items[itemCount].type = ITEM_SHOTGUN;
            items[itemCount].model = sgModel;
            items[itemCount].pos = entities[i].origin;
            items[itemCount].pos.y += 0.8f; // sinks into ground without this
            itemCount++;
        }
        else if(strcmp(entities[i].className,"ammo_shotgun")==0)
        {
            memset(&items[itemCount], 0, sizeof(Item));
            items[itemCount].type = ITEM_AMMO_SHOTGUN;
            items[itemCount].model = sgAmmoModel;
            items[itemCount].pos = entities[i].origin;
            //items[itemCount].pos.y += 0.8f; // sinks into ground without this
            itemCount++;
        }
        else if(strcmp(entities[i].className,"func_detail_wall")==0)
        {
            memset(&objects[objCount], 0, sizeof(EnvObject));
            objects[objCount].pointEntity = false;
            objects[objCount].model = entities[i].model;
            objects[objCount].model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = roofTexture;
            objCount++;
        }
        else
        {
            printf("no classname recognized for %s, entity %d, defaulting to worldspawn_ground. \n",entities[i].className,i);
        }
    }
    
    //put the lists into heap
    level.obj = MemAlloc(sizeof(EnvObject) * objCount);
    memcpy(level.obj, objects, sizeof(EnvObject) * objCount);
    level.objCount = objCount;

    level.bg = MemAlloc(sizeof(Enemy) * bgCount);
    memcpy(level.bg, badguys, sizeof(Enemy) * bgCount);
    level.bgCount = bgCount;

    level.items = MemAlloc(sizeof(Item) * itemCount);
    memcpy(level.items, items, sizeof(Item) * itemCount);
    level.itemCount = itemCount;

    
    //free the entities to prevent corruption later
    MemFree(entities);

    int totalEnvTri = 0;
    for(int i =0; i < level.objCount; i++)
    {
        if(level.obj[i].useOrigin && !level.obj[i].pointEntity){printf("object uses origin but is not point entity: %d ?\n",i);}
        totalEnvTri+=level.obj[i].model.meshes[0].triangleCount;
        level.obj[i].box = GetModelBoundingBox(level.obj[i].model);
        if(level.obj[i].useOrigin){level.obj[i].box=UpdateBoundingBox(level.obj[i].box,level.obj[i].origin);}
        level.obj[i].pos = PositionFromBox(level.obj[i].box);
        level.obj[i].radius = RadiusFromModelAndCenter(level.obj[i].pos,level.obj[i].model);
    }
    int totalBgTri = 0;
    for(int i =0; i < level.bgCount; i++)
    {
        if(level.bg[i].dead){printf("bg already dead: %d ?\n",i);}
        totalBgTri+=level.bg[i].model.meshes[0].triangleCount;
        BoundingBox orig = GetModelBoundingBox(level.bg[i].model);
        level.bg[i].origBox = orig;
        level.bg[i].box=UpdateBoundingBox(orig,level.bg[i].pos);
        level.bg[i].bodyBox=UpdateBoundingBox(level.bg[i].origBodyBox,level.bg[i].pos);
        level.bg[i].headBox=UpdateBoundingBox(level.bg[i].origHeadBox,level.bg[i].pos);
        level.bg[i].t_walk_stuck.virgin=false;
        level.bg[i].t_yeti_death_wait.virgin=false;
    }
    int totalItemTri = 0;
    for(int i =0; i < level.itemCount; i++)
    {
        if(level.items[i].isCollected){printf("item aleady collected: %d ?\n",i);}
        totalItemTri+=level.items[i].model.meshes[0].triangleCount;
        level.items[i].box=UpdateBoundingBox(GetModelBoundingBox(level.items[i].model),level.items[i].pos);
    }

    printf("Total Triangles for env objects: %d\n",totalEnvTri);
    printf("Total Triangles for bad guys   : %d\n",totalBgTri);
    printf("Total Triangles for items      : %d\n",totalItemTri);
    printf("Total Triangles                : %d\n",totalBgTri + totalEnvTri + totalItemTri);
    return level;
}

void UnloadLevel(Level * l)
{
    printf("unload models\n");
    //unique models
    for(int i=0;i<l->uniqueModels;i++)
    {
        UnloadModel(l->uModels[i]);
    }
    printf("unload bg models\n");
    //badguys store deep copies of thier models, unload each
    for(int i=0;i<l->bgCount;i++)
    {
        printf("attempting unload bg model %d/%d\n",i,l->bgCount);
        UnloadModel(l->bg[i].model);
    }
    printf("unload env obj models\n");
    //envObjects that do not use origin use unique models, unload each
    for(int i=0;i<l->objCount;i++)
    {
        if(!l->obj[i].pointEntity)//not a point entity that has a shared model
        {
            printf("attempting to unload Object %d/%d\n",i,l->objCount);
            UnloadModel(l->obj[i].model);
        }
    }
    printf("unload anims\n");
    //unique anims
    for(int i=0;i<l->uniqueAnimations;i++)
    {
        UnloadModelAnimations(l->uAnimations[i],l->uNumAnimations[i]);
    }
    printf("unload textures\n");//I was worried unload model unloads the texture, but that seems not the case
    //unique textures
    for(int i=0;i<l->uniqueTextures;i++)
    {
        UnloadTexture(l->uTextures[i]);
    }
    printf("unload sounds\n");
    //unique sounds
    for(int i=0;i<l->uniqueSounds;i++)
    {
        UnloadSound(l->uSounds[i]);
    }
    printf("MemFree unique lists\n");
    // Free dynamically allocated pointer arrays
    MemFree(l->uModels);
    MemFree(l->uAnimations);
    MemFree(l->uNumAnimations);
    MemFree(l->uTextures);
    MemFree(l->uSounds);
    printf("MemFree pointer lists\n");
    printf(" - items\n");
    MemFree(l->items);
    l->items = NULL;
    l->itemCount = 0;
    printf(" - enemies\n");
    MemFree(l->bg);
    l->bg = NULL;
    l->bgCount = 0;
    printf(" - objects\n");
    MemFree(l->obj);
    l->obj = NULL;
    l->objCount = 0;
    printf("clear fields\n");
    // Clear all fields
    *l = (Level){0};
    printf("mark as unloaded ...\n");
    l->loaded = false; // set this one explicetly
}