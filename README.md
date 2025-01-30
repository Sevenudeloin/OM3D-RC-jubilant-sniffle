# AUTHORS
- Ewan Lemonnier
- Axelle Mandy

# Summary
Project for EPITA (OM3D), flatland Radiance Cascades implementation in C++ using OpenGL.

## Setup
Requirements: cmake 3.20 minimum, C++17, and OpenGL 4.5.
```bash
# At the project root
mkdir -p build/debug
cd build/debug
cmake ../..
make
./OM3D
```
## Project structure

- `src/main.cpp` : Window, user inputs, GUI, pipeline steps (C++ part of the implem is written here)
- `src/*.h` & `src/*.cpp` : Helper classes (most of them unchanged from given codebase)
---
- `shaders/flatland_draw.comp` : Interactive drawing on screen from user
- `shaders/clear_draw_tex.comp` : Clear the draw texture
- `shaders/flatland_jfa_seed.comp` : Initialize seed for Jump Flooding Algorithm (JFA)
- `shaders/flatland_jfa.comp` : JFA Iteration based on seed to complete the final texture
- `shaders/flatland_jfa_dist.comp` : Conversion from JFA output to Signed Distance Field (SDF)
- `shaders/flatland_raymarch.comp` : Radiance Cascades implementation (should be named `flatland_rc.comp`)
- `shaders/screen.vert` & `shaders/flatland_render.frag` : Shader couple to display a texture on fullscreen triangle
---
The rest is legacy from other mini-projects that were done using this codebase prior to the RC implementation.

## Special thanks

https://radiance-cascades.com/ (a lot of resources on RC gathered here)

Sending much love to the Radiance Cascades discord community for proving so helpful and being lovely people :) 
