Sojourn

"Sojourn" is a 2D endless runner game built from scratch in C++ and Qt, inspired by the beautiful, "zen-like" aesthetic of games like Alto's Odyssey. This project is a deep dive into 2D game engine fundamentals, including procedural generation, physics, and rendering optimizations.

(Once you have a good screenshot, you can upload it to GitHub and link it here)

Features

This game is being built incrementally. The current engine features:

Custom Physics Engine: A simple physics implementation for gravity, jumping, and landing.

Procedural Terrain: The world is generated in real-time using Perlin Noise, making every run unique and endless.

Optimized Rendering: Uses a "sliding window" approach to generate new terrain segments ahead of the player and delete old ones, keeping performance high.

Parallax Scrolling: Multiple background layers move at different speeds to create a beautiful illusion of depth.

Skill Mechanics: Players can perform backflips in the air.

Collision Detection: Landing a flip incorrectly (e..g, on your head) or hitting an obstacle (rocks) will end the run.

Game State Management: A simple state machine handles the "Playing" and "Game Over" states, with a restart-on-key-press feature.

Built With

C++20

Qt 6 (QGraphicsView Framework): The entire game is built on Qt's powerful 2D scene graph. All rendering is self-contained within the Qt framework.

qmake

How to Build

This project uses qmake and can be built directly from Qt Creator.

Prerequisites:

Install Qt 6.x and Qt Creator from the official Qt website.

Make sure you have a C++ compiler (like GCC, MinGW, or MSVC) installed.

From Qt Creator (Recommended):

Open Sojourn.pro in Qt Creator.

Select the correct "Kit" (your compiler and Qt version).

Click the Build (hammer) icon, then the Run (green play) icon.

From Command Line:

# 1. Go to the project directory
cd path/to/Sojourn

# 2. Run qmake to generate the Makefile
qmake

# 3. Build the project
# On Windows (with MinGW)
mingw32-make
# On macOS/Linux
make

# 4. Run the executable
# On Windows
./debug/Sojourn.exe
# On macOS
./Sojourn.app/Contents/MacOS/Sojourn
# On Linux
./Sojourn


Next Steps

The next major feature in development is Phase 7: Immersive Scenery, which will add a full, dynamic day/night cycle to the game.
