# Scientific-Visualization

This repository contains assignments for a course in Scientific Visualization, implemented in C and OpenGL.
It demonstrates several visualization techniques including marching cubes, ray casting, Line Integral Convolution, Sammon mapping, and Self-Organizing Maps (SOM).

## Table of Contents
- [Overview](#overview)
- [Homework Assignments](#homework-assignments)
- [How to Run](#how-to-run)

## Overview

This project explores several scientific visualization techniques through a series of assignments:

- [**HW1: Iso-surface**](#hw1-iso-surface) — Marching Cubes — Extracting iso-surfaces from volumetric data.
- [**HW2: Ray Casting**](#hw2-ray-casting) — Direct volume rendering using ray tracing techniques.
- [**HW3: Line Integral Convolution (LIC)**](#hw3-line-integral-convolution-lic) — Visualizing vector fields with texture-based methods.
- [**HW4: Sammon Mapping**](#hw4-sammon-mapping) — Mapping scalar fields onto surfaces.
- [**HW5: Self-Organizing Maps (SOM)**](#hw5-self-organizing-maps-som) — Dimensionality reduction and clustering visualization.


## Homework Assignments

### Hw1: iso-surface
Extracting iso-surfaces from volumetric data using the Marching Cubes algorithm.

[View Report](https://github.com/user-attachments/files/21209972/report.pdf)

#### Main Interface Overview

<img src="iso_surface/result_image/engine_tri.JPG">

##### Histogram Panel
- Allows switching between displaying the **Original Histogram** or the **Log2-transformed Histogram**.  
  *The log2-transformed histogram makes it easier to identify appropriate iso-value regions.*
- Each dataset has a predefined iso-value, when **Line Mode** and **Multiple Iso-surface** are enabled, the colors of the lines on the histogram correspond to the iso-values of the surfaces.

##### Control Panel
- Adjust the camera position.
- Choose between two normal vector computation methods: **Triangle Normals** or **Gradient Normals**.
- Select between two rendering modes: **Fill Mode** or **Line Mode**.
- Enable **Multiple Iso-surface** display, and choose which surfaces to show (up to 3 surfaces; you can display one, two, or all three simultaneously).  
  *Line Mode provides clearer visualization of the surfaces.*
- Enable **Cross Section**, which offers two functions: **Cross Section** or **Slice**. You can select the direction of the cross section or slice and adjust its position.

##### File Panel
- Select different datasets to visualize.  
  *Switching datasets takes significant time due to recomputing the triangles.*
  
#### Results

##### Iso-surface Animation

<img src="MarchingCubes.gif">  

##### Carp Cross Section

<img src="iso_surface/result_image/carp_cross_section.JPG" width="300"> <img src="iso_surface/result_image/carp_cross_section_fill.JPG" width="300">

##### Engine Cross Section (Left) / Slice (Right)

<img src="iso_surface/result_image/engine_cross_section.JPG" width="300"> <img src="iso_surface/result_image/engine_slice.JPG" width="300">

##### Aneurism

<img src="iso_surface/result_image/aneurism.JPG" width="300">



### Hw2: ray-casting
Direct volume rendering using ray tracing techniques.

[View Report](https://github.com/user-attachments/files/21209974/report.pdf)

#### Results
<img src="ray_casting/result_image/engine1.JPG" width="300"> <img src="ray_casting/result_image/engine2.JPG" width="300">

<img src="ray_casting/result_image/fish.JPG" width="300"> <img src="ray_casting/result_image/fish2.JPG" width="300"> <img src="ray_casting/result_image/fish3.JPG" width="300">



### Hw3: Line Integral Convolution (LIC)
Visualizing vector fields using texture-based methods (LIC).

[View Report](https://github.com/user-attachments/files/21209991/report.pdf)

#### Results
<img src="Line_Integral_Convolution/result_image/vec13_black_box_off.JPG" width="300">  <img src="Line_Integral_Convolution/result_image/vec13_gray_box.JPG" width="300">

<img src="Line_Integral_Convolution/result_image/vec13_low_box_off.JPG" width="300">  <img src="Line_Integral_Convolution/result_image/vec13_low_tent_off.JPG" width="300">  <img src="Line_Integral_Convolution/result_image/vec13_spot_box_off.JPG" width="300">

<img src="Line_Integral_Convolution/result_image/vec19_color.JPG" width="300">  <img src="Line_Integral_Convolution/result_image/vec19_off.JPG" width="300">

<img src="Line_Integral_Convolution/result_image/vec7_color.JPG" width="300">  <img src="Line_Integral_Convolution/result_image/vec9.JPG" width="300">  



### Hw4: Sammon Mapping
Dimensionality reduction and visualization using Sammon mapping.

[View Report](https://github.com/user-attachments/files/21209998/report.pdf)

#### Results
<img src="Sammon_Mapping/result.JPG" width="400">

### Hw5: Self-Organizing Maps (SOM)
Clustering and dimensionality reduction visualization with SOM.

[View Report](https://github.com/user-attachments/files/21210002/report.pdf)

#### Results
<img src="SOM.gif">

## How to Run

### Requirements
- OpenGL
- GLUT or FreeGLUT
