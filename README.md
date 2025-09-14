# MicroPiano
MicroPiano is an RP2350-based Synthesizer and MIDI controller project under evelopment by ES@P. It features dynamic hot-swappable modules with magnetic pin connectors, and uses a per-key hall-effect system to dynamically determine key press velocity. 

## Design Components
**Work in progress!**

## IDE Setup
1. As a preliminary this project requires Visual Studio Code (VSCode), CMake (generally included on your OS of choice), and the Raspberry Pi Pico extension, available in VSCode in the 'Extensions' tab. 
2. Once cloning this repository, place it in the directory of your choice and open VSCode, selecting `File -> Open Folder` and selecting the folder containing the repository.
3. As of right now there is only one `master` branch, but this will be updated shortly to allow for a better development workflow. In the meantime, any changes you make can be observed locally, but no commits or changes can be made until more branches are made. 
4. All of the source code is contained in the major directory, under the `Conglomerate.c` file. *This will be updated in the future!*
5. If you want to make a project of your own for testing, under the Raspberry Pi Pico Project tab click on `New C/C++ Project` or `New Project from Example` and select all of the boilerplate you wish to use for testing.

---