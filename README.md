# BoomShocka FPS

This is a simple First Person Shooter game, written in c with raylib.

I am new to GameDev, this is my third game. I am so far about 4 weeks into the project.

I run on a raspberry pi4.

I heavily use ChatGPT and Meshy for development. Just wanted to note that clearly. Total vibe-coder over here.
I wouldnt be able to do this without help from ChatGPT, because of the math. Im not very good at linear algebra and
alot of the other concepts in game programming (and just c in general, I like c but I find it challenging).
ChatGPT also helps with properly utilizing raylib and with creating textures. 
And I am not very good at Blender, so I use Meshy for initial model/asset creation.
I use TrenchBroom for level creation.

This project requires raylib to be installed. See raylib.com (https://github.com/raysan5/raylib/wiki) for install and build instructions, and general guidance. Also, you might want to start by asking your favorite LLM how to get started, and adjusting things as needed.
 I have build.sh file and likely if you are also on a pi, it might work (possibly for other linux as well)
  - (cd BoomShockaFps; sh build.sh; ./game),

 I added web_build.sh, this is made for my setup but can possibly be easily changed.
  - textures are wrapped not repeated for the web, but I am mostly happy with them. Just wanted to note, it will look different for the web.
    - I adjusted texcoords to scale between 0-1, but that just wraps the whole thing, so then I added a hack to multiply the result by 5, and its better for most env objects. Not as good as native.
  - you will need to build raylib for web and setup emcc (ask AI and good luck, were all counting on you)
  - call like "bash web_build.sh", I cant remember why but this one has to be called with bash
  - change refs to ../raylib/src to whatever your path is for raylib/src folder
  - "source ../emsdk/emsdk_env.sh" will need to point correctly at you emsdk install
  - "export PATH=$HOME/binaryen/build/bin:$PATH" I dont think this is even correct for me right now, but if you have trouble, possibly adjust as needed
    - on my pi4, I had trouble with binaryen, it doesnt come out of the box for arm_64, if on a pi or similar you may need to clone it, and then build it (v123 for me), and even make the correct symlinks, ask ChatGPT for help, thats what I did, and also, if you are on a pi, it will build very slowly, like 30-60mins. Just a heads up.
  - sh serve.sh to run with python3 server
  - also, on my pi4 I get 30FPS at the best when running in a browser, I did try on a better computer and it was 60FPS, so maybe just an issue for us pi users out there.

I added a win_build.sh file for msys2 mingw64 on Windows10/11
 - install mysys2 "winget install --id MSYS2.MSYS2 -e", or maybe there are other ways not using winget, IDK
 - open a msys2 mingw64 terminal, has to be that  or the x86 one for x86 builds
 - install stuff with pacman
    - pacman -Syu
    - pacman -S mingw-w64-x86_64-raylib
    - pacman -S mingw-w64-x86_64-gcc
 - navigate to the BoomShockaFps folder, run "sh win_build.sh"
    - you might need to cd /c, before the path will show up correctly
    - uses a custom flag MEMORY_SAFE_MODE, all this changes is, for badguy models which I preffer to deep copy, it opens their file fresh for every bdaguy in the level, same result as deep copy but simpler and safer because raylib is not as flawed as my own code. I did this because while deep copy seems to work in linux and web, it seems c dev on Windows is actually more strict about proper memory managment, and I have some mistakes. I did try to fix them the right way first, went down the rabbit hole, got stuck and finally at 3AM thought of this. Just a note, this means on windows version, you will hit the file system alot more and it will take longer to load levels.
 - ./game.exe
    - you could copy all of the needed DLLs to the BoomShockaFps folder or wherever you want to keep the game.exe file, but otherwise, you cant just click on the exe file and run from file explorer or the like, you need to run it in mysys2 mingw64 for the DLL's to be found

I added mac_build.sh and mac_create_app.sh
 - on newer macs you can maybe go the brew install raylib route, but I am on an older Mac-mini, and had to build from source for both arm64 and x86_64, and then put things into the correct places, see the script for details of what is expected.
 - mac_build.sh output is best to just be run from the terminal like ./game. if you want to run it from finder, you should copy the asset folders over to your home directory (textures, models, maps, sounds). Additionally, the call to SetWorkingDirectoryToAppResources in main.c will work when built with mac_create_app.sh but should fail silently when built with mac_build.sh, 
  unless you were to add a Resources folder in /Users and copy the asset folders into it (Its probably better to just let it fail silently and run from a terminal). 

## controls
Esc on desktop/native is quit.
 - Esc on web uncaptures the mouse, but you might not be able to recapture it, so refresh to fix this (the game will restart tho)
    -  click the game screen to capture the mouse if it doesnt happen automatically

In game controls.
  - WASD to move
    - mouse to control the view, mouse capture is on
  - CTRL to crouch/stand up
  - Left mouse click to shoot
    - head shots are better
    - left and right arrow keys to change weapons
  - ENTER to pause

In Menu controls,
 - arrow up and down to select
 - ENTER to submit
 - in options D is toggle difficulty and Y is toggle y-inversion

## tools

My full pipeline for all of the stuff is TrenchBroom for level design 
(I have a .map parser, courtesy of AI of course, feel free to steal and improve, any of this really is fine to copy, modify, etc... 
I dont really know the rules but pretty sure I technically own everything and I am fine with sharing it with others),
Meshy for asset creation, Blender for animation (and sometimes model creation) and other corrections needed to get .glb files correct for raylib, and raylib is the final target framework. 

If you are interested in GameDev and want to get into these tools:
 - raylib - raylib.com (https://github.com/raysan5/raylib/wiki)
    - really nice framework for game dev
    - simple and yet suprisingly powerful even for 3D stuff
 - ChatGPT - ChatGPT.com (https://ChatGPT.com)
    - is a great cheat tool, expecially for harder 3D math
    - can also produce great textures
    - good for small to medium sized code snippets, it will give bad results sometimes tho
        - in those cases try to point out what is wrong at a high level
        - if it gets stuck, and just basically re-words the same response over and over, you will need to be the one with the a-ha moment to get things moving
        - in a similar vain, as your project grows, you will find you need to adapt what it gives you
 - Meshy - Meshy.ai (https://meshy.ai/)
    - really cool AI tool for asset generation and texturing
        - animations need work
        - doesnt do snakes well
        - will default to high poly counts so experiment and get a sense how low you can drop the count
            - you cant just say low poly in the description, 
            - when you finalize a choice of the four given options, you can pick poly count (3k or custom for even lower) 
        - does a great job with most prompts (I use text to model because I cant draw)
            - especially texturing, just wow, it does a fantastic job
            - textures are also, by default too large, you can resize them tho and the UV stuff should still match
                - also, for targeting raylib with .glb, you need to save the image and then add a texture image node under shading tab
 - Blender (https://www.blender.org/) - you probably know this one
    - I use this to rig, animate, and get .glb files ready to export to raylib
 - trenchbroom (https://trenchbroom.github.io/)
    - seriously cool level designer tool
        - much more simple than Blender because it is specialized
    - produces .map files, 
        - used in conjuction with the map_parser.c file, creates really simple but powerful and scale-able geometry
        - I use regular Quake .map files becuase they are simple
        - I use classname and subtype to control textures, raylib doesnt really support per face textures

## future enhancements, etc...

 TODO:
  - more levels
    - more textures and better use of those textures
    - better level design in general
  - more items
    - guns
        - also ammo for each gun
    - other items
  - Sounds
    - it feels empty, needs lots of sound effects
  - more and better enemies
    - better code around managing states and animations
    - yeti needs work
  - better collision detection
    - reduce wonky-ness
    - just better code in general, possibly shared code where possible
  - variable weather and time of day, at least background colors could change
  - objects that you can interact with like opening doors, etc...
  - Fix DeepCopyModel and DeepCopyMesh functions

The main thing that could be improved right now is my asset collection, 
some of them are way too big (the yeti is like 8k triangles).
I did not properly scale down textures that are baked into the glb files as well, and I am still figuring out Blender animation
especially in conjunction with Meshy assets.
  - (if any one knows how to get selected actions to stop going to NLA...seriously, I have tried over and over again and all of my Blender animation projects eventually end up in this endless loop of not being able to play animations from the Action Editor, and they go to NLA, but I delete all the strips from NLA, but then it just goes to NLA again...?). 
  
Rigging and assigning weights is still a problem for me, I havnt found exactly the process I like or what works well. 
 
 The yeti has really bad animation. I used a .py script in Blender to auto assign the weights and it worked okay. But I did a bad job correcting the result, so the yeti has limited range of motion off of his bones and as a result I did not create many animations for him and the ones I did are quite crappy. Also the yeti is trouble-some in general. He is super easy if you just shoot at the head, but really hard if you let him make it too you. Shooting him stops him in his tracks which looks odd and there is just many bugs to fix and small tweaks that I need to make. I will probably just deal with the large vertice count and texture size and just do a better job in the future.
 
 The soldier has okay animation but the models have cracks that appear as it moves because of bad weights (yes yes weght painting I know).
 Also this model is not centered correctly and my collision detection is fragile so if I make changes he often jitters or jumps.
 Overall, despite the issues, I am happy with the soldier and will probably not improve him much in this project.

 In general this project is a learning experience for me, I plan to do more games in the future and will take the lessons I learn here with me.