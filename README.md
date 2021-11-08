# gtreflect

gtreflect is an Application that is meant to be used as prebuild as well postbuild step on the scripts of each project, in order to produce the necessary files that [GreenTea Engine](https://github.com/VMormoris/GreenTea) needs for it's reflection system. This application is on a different repository from the engine because it uses [llvm-project](https://github.com/llvm/llvm-project) and although is a spectacular tool it's huge and thus I thought that is unnecessary to require you download and build it in order to be able to use the Engine.

## Prerequisites

* As mention above this repository uses [llvm-project](https://github.com/llvm/llvm-project) thus you will need to download and build it
* We also use [Premake5](https://github.com/premake/premake-core) for project generation so you will need to be able to run premake5 from the command line ([more info](https://github.com/VMormoris/GreenTea/wiki/Prerequisites#Premake5)).
* A Version Visual Studio that supports c++17

## Building

* Run: ```git clone --recursive https://github.com/VMormoris/gtreflect``` to the repository.
* Edit the premake5.lua file so ```llvmDir``` is pointing to the directory where you downloaded llvm.
* Run: ```premake5 vs20**``` where ** put the apropriate number for your Visual Studio version ([more info](https://premake.github.io/docs/Using-Premake))
* You can open the gtreflect.sln file and build using Visual Studio (Release version is required by the Engine)