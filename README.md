# Minecraft clone

A minecraft clone for me to practice 3d games & see how close I can get it to the real minecraft.

Uses OpenGL 3.3 and SDL for platform layer.

## Goals
- [x] Collision & physics
- [x] Block mining
- [x] Clouds & Sky 
- [x] Ambient Occulsion of blocks mult-threaded
- [x] Multi-threaded chunk generation
- [x] Tree & grasses generation
- [x] Water generation with alpha 
- [x] Health, Stamina & Item Hotbar
- [x] Coal & Ore generation & better world generation 
- [x] Load 3d models & skeletal animations from gltf file
- [ ] Load 3d models & animations in minecraft format form BlockBench (models don't use skeltal animation but multiple meshes per model and each mesh is animated separately). Combine all the meshes into one mesh, but give each mesh a index ID for the mesh they came from. Then we still make a 'skinning matrix' and the mesh is used to look up where it came from. Same as Bone Ids into the skinning matrix. 
- [ ] Cows, Sheep & Zombie Mobs 
- [ ] Snow and cliffs (learn more about simplex noise & SIMD library implementation of it)
- [ ] Attacking mobs 
- [ ] Cave generation
- [ ] Day & Night cycle
- [ ] Greedy meshing
- [ ] Shadow Mapping
- [ ] Point & Directional lights
- [ ] Server side application to run on a Rasberry Pi 4 that accepts TCP (block transactions) & UDP (entity positions) packets using Berkely Sockets API (learn about writing a server and running it, and learn about cyber security concepts in practice)
- [ ] Release game on steam as professional as possible - just exe & asset bundle 

## WHY's
- [x] Fun & Challenging to program
- [x] Get as good as I can at programming
- [x] Add to my projects list
- [x] Build confidence

![](screenshot2.png) 

## Building on Mac OS
run ```./run.sh``` to build and run the program. 

