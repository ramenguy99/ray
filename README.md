# Ray

Ray is a simple ray tracer written from scratch in C++ to learn about graphics programming.
It's written using a small subset of C++ very similar to C.

## Features
- Ray intersection with spheres, planes and triangle meshes
- Bounding volume hierarchies to speed up ray-mesh intersection
- Multithreading by rendering tiles of the output image in parallel
- Simple specular, refractive and emissive materials
- (0, 2)-sequence sampler for sample distribution and anti-aliasing

## Todo
- Phisically based materials
- Light and shadow samples
- Realistic camera model and depth of field

# Build
To build ray download the repository and open a shell inside it. The executable is created in the `build` directory and must be executed from there to find the resource files in `../res`

## Windows
Visual Studio must be installed to build on windows. 
```
cd build
build.bat

ray.exe out.bmp
```

## Linux
```
cd build
./build.sh

ray out.bmp
```

# Acknowledgements

The dragon model in `res/dragon.dae` is a reformat of the scan from Stanford University Computer Graphics Laboratory. Redistribution of the model is allowed for non commercial purposes. The original model and additional information is available at http://graphics.stanford.edu/data/3Dscanrep/

Some of the intersection tests and geometry procedures are from Real-Time Collision Detection, CRC Press 2004 by Christer Ericson

# Example

![Dragon and spheres](https://github.com/ramenguy99/ray/tree/master/images/dragon.png)