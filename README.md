A compute shader implementation of a jacobi iteration based fluid simulation as an **Unreal Engine** plugin. 

This is based on the same principles in [GPU Gems, Chapter 38](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu) and uses many other implementations as reference for the other specifics.

This simulation is in 3D space using a default grid size of 64x64x32 voxels, though this is user configurable, and takes approximates 500 micro seconds on a 3060 laptop GPU. Note that this is also using multiple superfluous texture readbacks, so there are optisations to make.

There are multiple sourcing types:  
**Fan** - Velocity in a direction.  
**Wake** - A moving object adds velocity in the direction of travel.  
**Explosion** - A radial explosion.  

The simulation outputs a velocity and density field which can both be used by other systems. 
The density field can be rendered using a heterogenous volume.
Niagara PFX systems can easily read from the velocity field to advect particles, this is very cool!

Future optimisations include using the methods outlined here:
https://www.youtube.com/watch?v=i07q2LcvI3c
https://www.youtube.com/watch?v=_3eyPUyqluc

