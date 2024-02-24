A simple cooperative multiplayer game, written in C++ using custom ECS implementation.

**Currently in early stages of development**

[![MIT License](https://img.shields.io/github/license/gameraccoon/tank-game)](https://github.com/gameraccoon/tank-game/blob/main/License.txt)  
[![Windows - build](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-windows.yml/badge.svg)](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-windows.yml) [![Ubuntu - build and run unit-tests](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-ubuntu.yml/badge.svg)](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-ubuntu.yml) [![Ubuntu - build with clang](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-ubuntu-clang.yml/badge.svg)](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-ubuntu-clang.yml)

## Intent
This is an educational project that doesn't have any applications to real-world problems, but it can still be used as a source of inspiration (or as a copypaste source).

This project was made in my free time for fun and to test some approaches. So don't expect much.

## Features
### Server-authoritative simulation with rollbacks
This is chosen mostly for educational purposes (since cheating in a real cooperative game may be not that a big problem). The server simulates the game based on input from the player, and the players only run the simulation for prediction. Any desynchronization results in the resimulation of incorrectly predicted frames with new data.

### Fully ECS-based code
ECS pattern has been chosen here **not** for performance reasons, but to get better maintainability and increase the testability of gameplay code. ECS makes these goals easier to achieve.

## Getting and building (Windows/Linux)
### Prerequisites for building the game
- git with git-lfs
- CMake (see the minimal supported version in [CMakeLists.txt](https://github.com/gameraccoon/tank-game/blob/main/CMakeLists.txt#L1=))
- python3
- GCC 13 (or higher), or clang-16 (or higher), or the latest Visual Studio 2022 (or newer)
- for Linux, you need to install dependencies of SLD2, also libssl, and protobuf using your packet manager  
e.g. for apt: `sudo apt-get install libopusfile-dev libflac-dev libxmp-dev libfluidsynth-dev libwavpack-dev libmodplug-dev libssl-dev libprotobuf-dev protobuf-compiler`

### Prerequisites for building the dedicated server
- git with git-lfs
- CMake (see the minimal supported version in [CMakeLists.txt](https://github.com/gameraccoon/tank-game/blob/main/CMakeLists.txt#L1=))
- python3
- GCC 13 (or higher), or clang-16 (or higher), or Visual Studio 2022 (or newer)
- for Linux, you need to install libssl and protobuf using your packet manager  
e.g. for apt: `sudo apt-get install libssl-dev libprotobuf-dev protobuf-compiler`

### Getting the code
With SSH  
`git clone --recursive git@github.com:gameraccoon/tank-game.git`

With HTTPS  
`git clone --recursive https://github.com/gameraccoon/tank-game.git`

### Building the game
#### Windows
For Windows with Visual Studio you can run `scripts\scripter\windows\generate_vs_project.cmd` it will generate the solution for Visual Studio 2022. Path to the generated solution: `build\game\GameMain.sln`

#### Linux
For all the other cases and platforms other than Windows just generate the project using CMake with `CMakeLists.txt` in the root folder.

E.g. using make
```bash
mkdir -p build/game
cd build/game
cmake ../..
make
```

After being built, the resulting executables can be found in `bin` folder

### Building a dedicated server
#### Windows
For Windows with Visual Studio you can run `scripts\generate_dedicated_server_vs2022_project.cmd` it will generate the solution for Visual Studio 2022. Path to the generated solution: `build\dedicated_server\DedicatedServer.sln`

#### Linux
For all the other cases and platforms other than Windows just generate the project using CMake as in the steps above but adding `-DDEDICATED_SERVER=ON`.

E.g. using make
```bash
mkdir -p build/dedicated_server
cd build/dedicated_server
cmake ../.. -DDEDICATED_SERVER=ON
make
```
