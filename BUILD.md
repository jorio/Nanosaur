# How to build the game

## TL;DR: Automated build script

Clone the repo **recursively**, then run `python3 build.py` to execute the build steps described in this document and package up the game.

build.py is the script that is used by the CI setup to produce builds. If you want to build the game manually instead, read on.

## How to build the game on macOS

1. Install the prerequisites:
    - Xcode 10+
    - [CMake](https://formulae.brew.sh/formula/cmake) 3.13+
1. Clone the repo recursively:
    ```
    git clone --recurse-submodules https://github.com/jorio/Nanosaur
    ```
1. Download [SDL2-2.0.18.dmg](http://libsdl.org/release/SDL2-2.0.18.dmg), open it, and copy **SDL2.framework** to the **extern** folder
1. Prep the Xcode project:
    ```
    cmake -G Xcode -S . -B build
    ```
1. Now you can open `build/Nanosaur.xcodeproj` in Xcode, or you can just go ahead and build the game:
    ```
    cmake --build build --config Release
    ```
1. The game gets built in `build/Release/Nanosaur.app`. Enjoy!

## How to build the game on Windows

1. Install the prerequisites:
    - Visual Studio 2019 with the C++ toolchain
    - [CMake](https://cmake.org/download/) 3.13+
1. Clone the repo recursively:
    ```
    git clone --recurse-submodules https://github.com/jorio/Nanosaur
    ```
1. Download [SDL2-devel-2.0.18-VC.zip](http://libsdl.org/release/SDL2-devel-2.0.18-VC.zip) and extract the contents into the **extern** folder
1. Prep the Visual Studio solution:
    ```
    cmake -G "Visual Studio 16 2019" -A x64 -S . -B build
    ```
1. Now you can open `build/Nanosaur.sln` in Visual Studio, or you can just go ahead and build the game:
    ```
    cmake --build build --config Release
    ```
1. The game gets built in `build/Release/Nanosaur.exe`. Enjoy!

## How to build the game on Linux et al.

1. Install the prerequisites from your package manager:
    - Any C++20 compiler
    - CMake 3.13+
    - SDL2 development library (e.g. "libsdl2-dev" on Debian/Ubuntu, "sdl2" on Arch)
    - glu
1. Clone the repo recursively:
    ```
    git clone --recurse-submodules https://github.com/jorio/Nanosaur
    ```
1. Build the game:
    ```
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build
    ```
1. The game gets built in `build/Release/Nanosaur`. Enjoy!

