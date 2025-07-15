# Scientific-Visualization

This repository contains assignments for a course in Scientific Visualization, implemented in C and OpenGL.
It demonstrates several visualization techniques including marching cubes, ray casting, Line Integral Convolution, Sammon mapping, and Self-Organizing Maps (SOM).

## Table of Contents
- [**Overview**](#overview)
- [**Homework Assignments**](#homework-assignments)
- [**How to Run**](#how-to-run)

## Overview

This project explores several scientific visualization techniques through a series of assignments:

- [**HW1: Iso-surface**](#hw1-iso-surface-extraction-marching-cubes) — Marching Cubes — Extracting iso-surfaces from volumetric data.
- [**HW2: Ray Casting**](#hw2-volume-rendering-using-ray-casting-gpu-accelerated) — Direct volume rendering using ray tracing techniques.
- [**HW3: Line Integral Convolution (LIC)**](#hw3-line-integral-convolution-lic-for-vector-field-visualization) — Visualizing vector fields with texture-based methods.
- [**HW4: Sammon Mapping**](#hw4-sammon-mapping) — Mapping scalar fields onto surfaces.
- [**HW5: Self-Organizing Maps (SOM)**](#hw5-self-organizing-maps-som) — Dimensionality reduction and clustering visualization.


## Homework Assignments

### Hw1: Iso-surface Extraction (Marching Cubes)
Implemented the **Marching Cubes** algorithm to extract iso-surfaces from volumetric data.

[View Report](https://github.com/user-attachments/files/21209972/report.pdf)

#### Key Techniques
- Support for multiple iso-values and line/fill rendering modes
- Camera control and normal vector calculation options
- Cross-section and slice visualization features
- Histogram panel for iso-value selection with log2 transformation
 
#### Main Interface Overview

<img src="iso_surface/result_image/engine_tri.JPG" width="800">

##### Histogram Panel
- Allows switching between displaying the **Original Histogram** or the **Log2-transformed Histogram**.  
  *The log2-transformed histogram makes it easier to identify appropriate iso-value regions.*
- Each dataset has a predefined iso-value, when **Line Mode** and **Multiple Iso-surface** are enabled, the colors of the lines on the histogram correspond to the iso-values of the surfaces.

##### Control Panel
- Adjust the camera position.
- Choose between two normal vector computation methods: **Triangle Normals** or **Gradient Normals**.
- Select between two polygon modes: **Fill Mode** or **Line Mode**.
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

<img src="iso_surface/result_image/carp_cross_section.JPG" width="500"> <img src="iso_surface/result_image/carp_cross_section_fill.JPG" width="500">

##### Engine Cross Section (Left) / Slice (Right)

<img src="iso_surface/result_image/engine_cross_section.JPG" width="500"> <img src="iso_surface/result_image/engine_slice.JPG" width="500">

##### Aneurism

<img src="iso_surface/result_image/aneurism.JPG" width="500">



### Hw2: Volume Rendering using Ray Casting (GPU Accelerated)

Implemented direct volume rendering using **ray casting** accelerated by GPU. 

[View Report](https://github.com/user-attachments/files/21209974/report.pdf)

#### Key Techniques
- Adjustable ray step gap for performance/quality tradeoff
- Phong shading mode support
- Interactive camera manipulation
- Color transfer function editor for dynamic visualization

#### Main Interface Overview

##### Histogram Panel

##### File Panel

##### Control Panel
- Adjust the camera position.
- Adjust the **Ray Casting Gap Size**, which controls the sampling interval during ray tracing.
- Enable or disable **Phong Shading Mode** for enhanced surface shading.

##### Transfer Panel
- Configure the **Transfer Function** for color and opacity mapping.
- Add new color segments by inputting colors and defining ranges.
- Remove the most recently added color segment.

#### Results

##### Add Color Segments on Engine

<img src="ray_casting/result_image/engine1.JPG" width="500"> <img src="ray_casting/result_image/engine2.JPG" width="500">

##### Different Gap on Fish

<img src="ray_casting/result_image/fish.JPG" width="500"> <img src="ray_casting/result_image/fish2.JPG" width="500"> <img src="ray_casting/result_image/fish3.JPG" width="500">



### Hw3: Line Integral Convolution (LIC) for Vector Field Visualization

Visualized 2D vector fields by convolving noise textures along streamlines using LIC. 

[View Report](https://github.com/user-attachments/files/21209991/report.pdf)

#### Key Techniques

- Support for different noise types: black & white, gray-scale, spot noise
- Multiple convolution kernels: box filter, tent filter
- Color mode based on relative velocity magnitude
- Side-by-side display of noise and LIC results
  
#### Main Interface Overview

##### Control Panel
- Select different **Noise Types**.
- Select different **Convolution Filter**.
- Adjust the **Convolution Mask Size(Convolution Kernel Size)**.
- Enable **Color Display Mode**, where colors represent relative velocity magnitudes.

##### File Panel

##### Display Panel
- The **left panel** shows the visualization result.
- The **right panel** shows the noise texture.

#### Results

##### vec13 — High-Frequency Noise: Black & White (Left) / Gray (Right), Different Noise Types

<img src="Line_Integral_Convolution/result_image/vec13_black_box_off.JPG" width="500"> <img src="Line_Integral_Convolution/result_image/vec13_gray_box.JPG" width="500">

##### vec13 — Low-Frequency Noise: Box Filter (Left) / Tent Filter (Right), Different Convolution Filters

<img src="Line_Integral_Convolution/result_image/vec13_low_box_off.JPG" width="500"> <img src="Line_Integral_Convolution/result_image/vec13_low_tent_off.JPG" width="500">

##### vec13 — Spot Noise

<img src="Line_Integral_Convolution/result_image/vec13_spot_box_off.JPG" width="500">

##### Other Data Results

<img src="Line_Integral_Convolution/result_image/vec19_color.JPG" width="500"> <img src="Line_Integral_Convolution/result_image/vec7_color.JPG" width="500"> <img src="Line_Integral_Convolution/result_image/vec9.JPG" width="500">



### Hw4: Sammon Mapping
Dimensionality reduction and visualization using Sammon mapping.

[View Report](https://github.com/user-attachments/files/21209998/report.pdf)

#### Results
<img src="Sammon_Mapping/result.JPG" width="400">



### HW5: Self-Organizing Maps (SOM)

Implemented **Self-Organizing Map (SOM)** trained on cylindrical lattices to generate a vase-shaped structure.

[**View Report**](https://github.com/user-attachments/files/21210002/report.pdf)

#### Key Techniques

- Adaptive learning rate
- Dynamic neighborhood layers decreasing over iterations
  
#### Training Parameters

The SOM training process uses the following parameters and rules:

1. Learning Rate

   The learning rate at each iteration is computed as:
   
   `delta = 0.01 * exp(-iteration / max_iteration)`

2. Neighborhood Layers

   The number of neighborhood layers `t` depends on the current iteration:
   
   ```
   if iteration > max_iteration / 2 then t = 1
   
   else if iteration > max_iteration / 5 then t = 2
   
   else if iteration > max_iteration / 10 then t = 3
   
   else t = 4
   ```

3. Neighborhood Function

   The influence of a node at distance `d` is computed as:
   
   `r(d) = 1.0 / sqrt(d)`

#### Results

<img src="SOM.gif" width="400">



## How to Run

### Development Environment

- Visual Studio 2022 or later  
- OpenGL 3.3 or higher support

### Folder Structure

- `header/`  
  Contains project-specific header files such as `shader.h`, `glad.h`, and `imgui.h`.

- `include/`  
  Contains third-party libraries headers including `GLFW`, `KHR`, `glad`, and `glm`.

- `lib-vc2022/`  
  Contains precompiled `.lib` files required for linking (compatible with Visual Studio 2022).  

### Instructions

1. Open Visual Studio and select **Open Folder**, then choose this project’s root directory.  
2. In Visual Studio, add `header/` and `include/` folders to the **Additional Include Directories** in the project properties.  
3. Make sure OpenGL and GLFW libraries are installed on your system. Link the necessary `.lib` files in the **Linker** settings.  
4. Build and run the project.  
5. Each homework assignment is located in a separate folder; open and run the desired assignment accordingly.


