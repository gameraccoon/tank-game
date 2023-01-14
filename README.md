A simple cooperative multiplayer game written on C++ using custom ECS implementation.

**Currently in early stages of development**

[![MIT License](https://img.shields.io/github/license/gameraccoon/tank-game)](https://github.com/gameraccoon/tank-game/blob/main/License.txt)  
[![Windows - build](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-windows.yml/badge.svg)](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-windows.yml) [![Ubuntu - build and run unit-tests](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-ubuntu.yml/badge.svg)](https://github.com/gameraccoon/tank-game/actions/workflows/build-game-ubuntu.yml)

## Intent
This is an educational project that doesn't have any applications to real-world problems, but it can still be used as a source of inspiration (or as a copypaste source).

This project was made in my free time for fun and to test some approaches. So don't expect much.

## Features
### Server-authoritative simulation with rollbacks
This is choosen mostly for educational purposes (since cheating in a real cooperative game may be not that a big problem). The server simulates the game based on input from the player, and the players only run the simulation for prediction. Any desynchronization results in resimulation of incorrectly predicted frames with new data.

### Fully ECS-based code
ECS pattern has been chosen here **not** for performance reasons, but to get better maintainability and increase testability of gameplay code. ECS makes these goals easier to be achieved.

## Getting and building (Windows/Linux)
### Prerequisites for building the game
- git with git-lfs
- CMake (see minimal supported version in [CMakeLists.txt](https://github.com/gameraccoon/tank-game/blob/main/CMakeLists.txt#L1=))
- python3
- gcc 11 (or higher) or latest Visual Studio 2022 (VS 2019 may work but it is not automatically tested on CI)
- for Linux you need to install SLD2, SDL2_Image and SDL2_mixer (devel and static), libssl and protobuf using your packet manager  
e.g. for apt: `sudo apt-get install libsdl2-2.0-0 libsdl2-dev libsdl2-image-2.0-0 libsdl2-image-dev libsdl2-mixer-2.0-0 libsdl2-mixer-dev libsdl2-ttf-2.0-0 libsdl2-ttf-dev libssl-dev libprotobuf-dev protobuf-compiler`

### Prerequisites for building the dedicated server
- git with git-lfs
- CMake (see minimal supported version in [CMakeLists.txt](https://github.com/gameraccoon/tank-game/blob/main/CMakeLists.txt#L1=))
- python3
- gcc 11 (or higher) or latest Visual Studio 2022 (VS 2019 may work but it is not automatically tested on CI)
- for Linux you need to install libssl and protobuf using your packet manager  
e.g. for apt: `sudo apt-get install libssl-dev libprotobuf-dev protobuf-compiler`

### Getting the code
With SSH  
`git clone --recursive git@github.com:gameraccoon/tank-game.git`

With HTTPS  
`git clone --recursive https://github.com/gameraccoon/tank-game.git`

### Building the game
#### Windows
For Windows with Visual Studio you can run `scripts\generate_game_vs2020_project.cmd` it will generate the solution for Visual Studio 2022. Path to the generated solution: `build\game\GameMain.sln`

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

### Building dedicated server
#### Windows
For Windows with Visual Studio you can run `scripts\generate_dedicated_server_vs2020_project.cmd` it will generate the solution for Visual Studio 2022. Path to the generated solution: `build\dedicated_server\DedicatedServer.sln`

#### Linux
For all the other cases and platforms other than Windows just generate the project using CMake as in the steps above but adding `-DDEDICATED_SERVER=ON`.

E.g. using make
```bash
mkdir -p build/dedicated_server
cd build/dedicated_server
cmake ../.. -DDEDICATED_SERVER=ON
make
```
