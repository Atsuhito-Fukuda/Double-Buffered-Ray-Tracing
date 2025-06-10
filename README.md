# Source Code for Double Buffered Ray Tracing

Double Buffered Ray Tracing is a novel method for Ray Tracing.

Define a shape by the region enclosed within multiple infinite planes and cast two rays from both the incident and its opposite sides to capture it.
The capturings are determined by testing if the rays to each plane intersection point cross each other.

Furthermore, the method also presents constructing complex shapes by subtracting a portion of a shape from another.

<div align="center">
  <img src="https://github.com/Atsuhito-Fukuda/Double-Buffered-Ray-Tracing/releases/download/v1.0.0/introduction.png" alt="concise visual summary of DB Ray Tracing" style="width:65%;"> <br>
  <span style="font-size:14px;">
    A concise visual summary of the method's framework. (a) Two rays, facing each other, <br>
    obtaining a shape defined by infinite planes. (b) A rendering of a cube from which <br>
    the lower part of an octahedron has been subtracted, obtained with two rays.
  </span>
</div> <br> <br>


This source code demonstrates the novel method, processing the "Ray2" and "Selection" calculations for "Ray Units/Objects".

Rays are used as camera rays in this demonstration.

The results of the calculations appear as Ray Units/Objects drawn on the screen.

## Documentation

- [Main Article (preprint, IEEE TVCG, PDF)](https://github.com/Atsuhito-Fukuda/Double-Buffered-Ray-Tracing/releases/download/v1.0.0/Double_Buffered_Ray_Tracing.preprint.pdf) is the main (preprint) article, which presents the concept, calculations, and advantages/disadvantages of the novel method.
  
- [Ray Calculation Workflow (PDF)](https://github.com/Atsuhito-Fukuda/Double-Buffered-Ray-Tracing/releases/download/v1.0.0/RayCalcWorkflow.pdf) presents the computer-based workflow of the ray calculations in the novel method.


## Build

This repository contains projects for Windows Visual Studio and macOS.

### Windows Visual Studio

- Download the project through the Visual Studio branch and build it in Visual Studio.

### macOS

- Download the project through the macOS branch and compile using clang++.

```bash

cd ./ray/

clang++ --std c++14 -I./exlinks/include/ -L./exlinks/lib/ -framework OpenGL -framework CoreFoundation -lglfw.3 -lGLEW  -lSOIL -rpath ./exlinks/lib/ ./src/*.cpp --output ./Ray

```

- Make sure the Xcode Command Line Tools are installed.

### Dependencies

The following necessary third-party libraries and their header files are included in `./ray/exlinks/`.

- OpenGL
- GLEW (The OpenGL Extension Wrangler Library, [License](https://github.com/nigels-com/glew#copyright-and-licensing).)
- GLFW ([License](https://www.glfw.org/license.html))
- SOIL (Simple OpenGL Image Library, [License (Archive)](https://web.archive.org/web/20200728145723/http://lonesock.net/soil.html))


## Customization

- Edit shaders in `./ray/src/sh/` to modify the ray calculations.

- Type, number, parameters of Ray Units/Objects can be edited in the Ray class constructor in `./ray/src/Ray.cpp`.

## Mouse / Keyboard Controls

When you run the application, you can rotate the camera with the mouse and adjust its position using the following keys:

- Up Arrow: Move forward
- Down Arrow: Move backward
- Left Arrow: Move left
- Right Arrow: Move right
- Alt (option): Move up
- Ctrl (control): Move down
