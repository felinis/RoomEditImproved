![Project-eden-logo](https://github.com/felinis/RoomEditImproved/assets/94763702/3da3780a-0894-454b-8f1a-4e065329b44a)

# Overview
WIP level editor for Project Eden, a video game released originally in 2001 by the British studio Core Design.
The program is written in high-quality C++.

# Rendering Technology
The program uses a modern and powerful Direct3D 12 renderer. Video RAM and GPU heaps are managed manually for maximum performance.
On top of that, I implemented _bindless textures_, which is a powerful feature allowing for faster texture streaming.

# The User Interface
The program sports a lightweight UI, which was programmed using the native Windows API (Win32).

# Screens
This screen shows the program user interface with a temple level loaded in. The user can add and remove objects, as well as alter their various attributes on the panel on the right.
<img width="948" alt="up" src="https://github.com/felinis/RoomEditImproved/assets/94763702/69db9ec7-f0ed-44d2-96a8-2915ea365020">

# Building Instructions
- Install Visual Studio (2017 is recommended). I haven't tested MinGW, but make sure it supports C++17.
- Install CMake (minimum 3.12).
- Configure the CMakeLists.txt for the Visual Studio compiler. I personally used Visual Studio 2017 Professional.
- Build.
- Use the `edndec.exe` tool to decompress the level files, then open them inside Room Editor.
