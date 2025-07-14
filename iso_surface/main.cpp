#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <GL/glut.h>
#include <stdlib.h> 
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include "imgui.h"
#include "imgui_impl_glut.h"
#include "imgui_impl_opengl2.h"
using namespace std;
#define EPS 1e-12

struct xyz {
    float x, y, z;
    xyz() {
        x = 0, y = 0, z = 0;
    }
    xyz(float x_, float y_, float z_) {
        x = x_, y = y_, z = z_;
    }
};
struct triangle {
    xyz p[3];
    xyz n1; // normal - method 1
    xyz n2[3]; // normal - method 2
};
struct gridcell {
    xyz p[8];
    float val[8] = { 0 };
};
static const int offset[8][3] = {
        {0, 0, 0},{1, 0, 0},{1, 1, 0},{0, 1, 0},
        {0, 0, 1},{1, 0, 1},{1, 1, 1},{0, 1, 1}
};
static const int edge_connection[12][2] = {
        {0,1}, {1,2}, {2,3}, {3,0},
        {4,5}, {5,6}, {6,7}, {7,4},
        {0,4}, {1,5}, {2,6}, {3,7}
};
static const float edge_direction[12][3] = {
        {1.0, 0.0, 0.0},{0.0, 1.0, 0.0},{-1.0, 0.0, 0.0},{0.0, -1.0, 0.0},
        {1.0, 0.0, 0.0},{0.0, 1.0, 0.0},{-1.0, 0.0, 0.0},{0.0, -1.0, 0.0},
        {0.0, 0.0, 1.0},{0.0, 0.0, 1.0},{ 0.0, 0.0, 1.0},{0.0,  0.0, 1.0}
};

// Data and gradient parameter
const int L = 256, W = 256, H = 256;
unsigned char testing_data[L][W][H];
xyz gradient[L][W][H];
// Calculation for triangle parameter
triangle cur_tri;
triangle triangles[3][6000000];
int tri_idx[] = { 0,0,0 };
int normal_function = 1;
gridcell grid;
float isolevel[6][3] = {
    { 50,100,150 }, {8, 90, 150},{10, 150, 170},{50, 100, 220},{25, 50, 100}, {40, 70, 100} };
// Draw parameter
float width = 1080.0;
float height = 720.0;
GLenum  polygon_mode = GL_FILL;
float background_color[3] = { 10 / 255.0 ,10 / 255.0 ,70 / 255.0 };
float triangle_color[3][3] = { {0, 1.0, 1}, {0, 0.5, 1.0}, {0, 0, 1.0} };
// Eye parameter
float eye_coord[4][3] = {
    {250, 0, 0},{0, 250, 0},{0, 0, 250} ,{252, 152, 272} };
float eye_look[3] = { 128, 128, 128 };
int eye_spot = 0;
// Light source parameter
float light0_position[] = { 0, 200, 0, 1 };
float light1_position[] = { 200, 0, 0, 1 };
float light2_position[] = { 0, 200, 0, 1 };
float light3_position[] = { 0, 0, 200, 1 };
float light0_diffuse[] = { 0.8, 0.8, 0.8, 1.0 };
float light0_specular[] = { 0.1, 0.1, 0.1, 1.0 };
float light0_ambient[] = { 0.1,0.1,0.1,1.0 };
float light0_angle = 0;
bool light0_on = FALSE, light1_on = FALSE, light2_on = TRUE, light3_on = FALSE;
// Cross section parameter
int cross_slice = 0; // 0:off, 1:cross section 2:slice
int cross_section_mode = 0; // for direction
float cross_section_position = 80.0; // only for cross section not for slice
// Multiple iso-surface parameter
bool multiple_surface = false;
int surface_cnt = 0;
int choose_surface = 3;
// Histogram parameter
float histogram_data[2][256] = { 0 }; // 0:origin 1:log2
// 2D draw parameter
double vertex[10][2];
// File parameter
int file_num = 0;
const char* filename[6] = { "testing_engine.raw", "foot.raw", "Carp.raw", "bonsai.raw", "aneurism.raw" , "skull.raw" };


xyz VertexInterp(float isolevel, xyz p1, xyz p2, float valp1, float valp2) {
    float mu;
    xyz p;
    if (abs(isolevel - valp1) < 0.00001)return(p1);
    if (abs(isolevel - valp2) < 0.00001)return(p2);
    if (abs(valp1 - valp2) < 0.00001)return(p1);
    mu = (isolevel - valp1) / (valp2 - valp1);
    p.x = p1.x + mu * (p2.x - p1.x);
    p.y = p1.y + mu * (p2.y - p1.y);
    p.z = p1.z + mu * (p2.z - p1.z);
    return(p);
}
// Method 1: normal verctor = triangle cross product 
void Calculate_normal1() {
    triangle t = cur_tri;
    xyz u(t.p[0].x - t.p[1].x, t.p[0].y - t.p[1].y, t.p[0].z - t.p[1].z);
    xyz v(t.p[0].x - t.p[2].x, t.p[0].y - t.p[2].y, t.p[0].z - t.p[2].z);
    xyz outer_product(u.y * v.z - u.z * v.y, u.z * v.x - u.x * v.z, u.x * v.y - u.y * v.x);
    float sum = sqrt(outer_product.x * outer_product.x + outer_product.y * outer_product.y + outer_product.z * outer_product.z);
    outer_product.x /= sum;
    outer_product.y /= sum;
    outer_product.z /= sum;
    cur_tri.n1 = outer_product;
}
// Method 2: by gradient
void Calculate_normal2(vector<int>& edge, gridcell grid) {
    // Linearly interpolate gradients along edges to compute the gradients of the intersections
    // Interpolate in (x, triangle.x, x+1) and (gradient[x], [value], gradient[x+1])
    float diff_x, diff_y, diff_z;
    for (int i = 0; i < 3; i++) {
        int id0 = edge_connection[edge[i]][0], id1 = edge_connection[edge[i]][1]; // vertex id oncube
        xyz p0(grid.p[0].x + offset[id0][0], grid.p[0].y + offset[id0][1], grid.p[0].z + offset[id0][2]);
        xyz p1(grid.p[0].x + offset[id1][0], grid.p[0].y + offset[id1][1], grid.p[0].z + offset[id1][2]);
        int x0 = (int)p0.x, y0 = (int)p0.y, z0 = (int)p0.z;
        int x1 = (int)p1.x, y1 = (int)p1.y, z1 = (int)p1.z;
        diff_x = gradient[x0][y0][z0].x + (cur_tri.p[i].x - x0) * (gradient[x1][y1][z1].x - gradient[x0][y0][z0].x);
        diff_y = gradient[x0][y0][z0].y + (cur_tri.p[i].y - y0) * (gradient[x1][y1][z1].y - gradient[x0][y0][z0].y);
        diff_z = gradient[x0][y0][z0].z + (cur_tri.p[i].z - z0) * (gradient[x1][y1][z1].z - gradient[x0][y0][z0].z);
        cur_tri.n2[i] = xyz(-diff_x, -diff_y, -diff_z);
    }
}
void Calculate_gradient() {
    xyz diff;
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < W; j++) {
            for (int k = 0; k < H; k++) {
                if (i > 0 && i < L - 1 && j > 0 && j < W - 1 && k > 0 && k < H - 1) {// central difference
                    diff.x = ((float)testing_data[i + 1][j][k] - (float)testing_data[i - 1][j][k]) / 2.0;
                    diff.y = ((float)testing_data[i][j + 1][k] - (float)testing_data[i][j - 1][k]) / 2.0;
                    diff.z = ((float)testing_data[i][j][k + 1] - (float)testing_data[i][j][k - 1]) / 2.0;
                }
                else if (i == 0 || j == 0 || k == 0) {// forward difference
                    diff.x = ((float)testing_data[i + 1][j][k] - (float)testing_data[i][j][k]);
                    diff.y = ((float)testing_data[i][j + 1][k] - (float)testing_data[i][j][k]);
                    diff.z = ((float)testing_data[i][j][k + 1] - (float)testing_data[i][j][k]);
                }
                else {// backward difference
                    diff.x = ((float)testing_data[i][j][k] - (float)testing_data[i - 1][j][k]);
                    diff.y = ((float)testing_data[i][j][k] - (float)testing_data[i][j - 1][k]);
                    diff.z = ((float)testing_data[i][j][k] - (float)testing_data[i][j][k - 1]);
                }
                // normalize
                float temp = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
                if (temp >= EPS) {
                    diff.x /= temp;
                    diff.y /= temp;
                    diff.z /= temp;
                }
                gradient[i][j][k] = diff;
            }
        }
    }
}
// Return the number of triangular facets
int edgeTable[256] = {
    0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
    0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
    0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
    0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
    0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
    0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
    0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
    0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
    0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
    0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
    0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
    0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
    0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
    0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
    0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
    0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
    0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
    0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
    0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
    0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
    0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
    0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
    0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
    0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
    0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
    0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
    0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
    0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
    0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
    0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
    0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
    0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0 };
int triTable[256][16] = {
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 9, 8, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 0, 2, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 8, 3, 2, 10, 8, 10, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 8, 11, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 2, 1, 9, 11, 9, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 1, 11, 10, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 10, 1, 0, 8, 10, 8, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{3, 9, 0, 3, 11, 9, 11, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 7, 3, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 1, 9, 4, 7, 1, 7, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 4, 7, 3, 0, 4, 1, 2, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 2, 10, 9, 0, 2, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{8, 4, 7, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 4, 7, 11, 2, 4, 2, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 8, 4, 7, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1, -1, -1, -1, -1},
{3, 10, 1, 3, 11, 10, 7, 8, 4, -1, -1, -1, -1, -1, -1, -1},
{1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4, -1, -1, -1, -1},
{4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{4, 7, 11, 4, 11, 9, 9, 11, 10, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 1, 5, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 5, 4, 8, 3, 5, 3, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 10, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 2, 10, 5, 4, 2, 4, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8, -1, -1, -1, -1},
{9, 5, 4, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 11, 2, 0, 8, 11, 4, 9, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 5, 4, 0, 1, 5, 2, 3, 11, -1, -1, -1, -1, -1, -1, -1},
{2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5, -1, -1, -1, -1},
{10, 3, 11, 10, 1, 3, 9, 5, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10, -1, -1, -1, -1},
{5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3, -1, -1, -1, -1},
{5, 4, 8, 5, 8, 10, 10, 8, 11, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 5, 7, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 3, 0, 9, 5, 3, 5, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 8, 0, 1, 7, 1, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 7, 8, 9, 5, 7, 10, 1, 2, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3, -1, -1, -1, -1},
{8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2, -1, -1, -1, -1},
{2, 10, 5, 2, 5, 3, 3, 5, 7, -1, -1, -1, -1, -1, -1, -1},
{7, 9, 5, 7, 8, 9, 3, 11, 2, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7, -1, -1, -1, -1},
{11, 2, 1, 11, 1, 7, 7, 1, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11, -1, -1, -1, -1},
{5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0, -1},
{11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0, -1},
{11, 10, 5, 7, 11, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 8, 3, 1, 9, 8, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 2, 6, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 5, 1, 2, 6, 3, 0, 8, -1, -1, -1, -1, -1, -1, -1},
{9, 6, 5, 9, 0, 6, 0, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 0, 8, 11, 2, 0, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11, -1, -1, -1, -1},
{6, 3, 11, 6, 5, 3, 5, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9, -1, -1, -1, -1},
{6, 5, 9, 6, 9, 11, 11, 9, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 3, 0, 4, 7, 3, 6, 5, 10, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 5, 10, 6, 8, 4, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4, -1, -1, -1, -1},
{6, 1, 2, 6, 5, 1, 4, 7, 8, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7, -1, -1, -1, -1},
{8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6, -1, -1, -1, -1},
{7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9, -1},
{3, 11, 2, 7, 8, 4, 10, 6, 5, -1, -1, -1, -1, -1, -1, -1},
{5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11, -1, -1, -1, -1},
{0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6, -1, -1, -1, -1},
{9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6, -1},
{8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6, -1, -1, -1, -1},
{5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11, -1},
{0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7, -1},
{6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9, -1, -1, -1, -1},
{10, 4, 9, 6, 4, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 10, 6, 4, 9, 10, 0, 8, 3, -1, -1, -1, -1, -1, -1, -1},
{10, 0, 1, 10, 6, 0, 6, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{1, 4, 9, 1, 2, 4, 2, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4, -1, -1, -1, -1},
{0, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 3, 2, 8, 2, 4, 4, 2, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 4, 9, 10, 6, 4, 11, 2, 3, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6, -1, -1, -1, -1},
{3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10, -1, -1, -1, -1},
{6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1, -1},
{9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3, -1, -1, -1, -1},
{8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1, -1},
{3, 11, 6, 3, 6, 0, 0, 6, 4, -1, -1, -1, -1, -1, -1, -1},
{6, 4, 8, 11, 6, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 10, 6, 7, 8, 10, 8, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10, -1, -1, -1, -1},
{10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0, -1, -1, -1, -1},
{10, 6, 7, 10, 7, 1, 1, 7, 3, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9, -1},
{7, 8, 0, 7, 0, 6, 6, 0, 2, -1, -1, -1, -1, -1, -1, -1},
{7, 3, 2, 6, 7, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7, -1, -1, -1, -1},
{2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7, -1},
{1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11, -1},
{11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1, -1, -1, -1, -1},
{8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6, -1},
{0, 9, 1, 11, 6, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0, -1, -1, -1, -1},
{7, 11, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 8, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 9, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 9, 8, 3, 1, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{10, 1, 2, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 8, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{2, 9, 0, 2, 10, 9, 6, 11, 7, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8, -1, -1, -1, -1},
{7, 2, 3, 6, 2, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{7, 0, 8, 7, 6, 0, 6, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{2, 7, 6, 2, 3, 7, 0, 1, 9, -1, -1, -1, -1, -1, -1, -1},
{1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6, -1, -1, -1, -1},
{10, 7, 6, 10, 1, 7, 1, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8, -1, -1, -1, -1},
{0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7, -1, -1, -1, -1},
{7, 6, 10, 7, 10, 8, 8, 10, 9, -1, -1, -1, -1, -1, -1, -1},
{6, 8, 4, 11, 8, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 3, 0, 6, 0, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 6, 11, 8, 4, 6, 9, 0, 1, -1, -1, -1, -1, -1, -1, -1},
{9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6, -1, -1, -1, -1},
{6, 8, 4, 6, 11, 8, 2, 10, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6, -1, -1, -1, -1},
{4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9, -1, -1, -1, -1},
{10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3, -1},
{8, 2, 3, 8, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 2, 4, 6, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8, -1, -1, -1, -1},
{1, 9, 4, 1, 4, 2, 2, 4, 6, -1, -1, -1, -1, -1, -1, -1},
{8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1, -1, -1, -1, -1},
{10, 1, 0, 10, 0, 6, 6, 0, 4, -1, -1, -1, -1, -1, -1, -1},
{4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3, -1},
{10, 9, 4, 6, 10, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 5, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 5, 11, 7, 6, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 1, 5, 4, 0, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5, -1, -1, -1, -1},
{9, 5, 4, 10, 1, 2, 7, 6, 11, -1, -1, -1, -1, -1, -1, -1},
{6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5, -1, -1, -1, -1},
{7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2, -1, -1, -1, -1},
{3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6, -1},
{7, 2, 3, 7, 6, 2, 5, 4, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7, -1, -1, -1, -1},
{3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0, -1, -1, -1, -1},
{6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8, -1},
{9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7, -1, -1, -1, -1},
{1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4, -1},
{4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10, -1},
{7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10, -1, -1, -1, -1},
{6, 9, 5, 6, 11, 9, 11, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5, -1, -1, -1, -1},
{0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11, -1, -1, -1, -1},
{6, 11, 3, 6, 3, 5, 5, 3, 1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6, -1, -1, -1, -1},
{0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10, -1},
{11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5, -1},
{6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3, -1, -1, -1, -1},
{5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2, -1, -1, -1, -1},
{9, 5, 6, 9, 6, 0, 0, 6, 2, -1, -1, -1, -1, -1, -1, -1},
{1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8, -1},
{1, 5, 6, 2, 1, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6, -1},
{10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0, -1, -1, -1, -1},
{0, 3, 8, 5, 6, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{10, 5, 6, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 7, 5, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{11, 5, 10, 11, 7, 5, 8, 3, 0, -1, -1, -1, -1, -1, -1, -1},
{5, 11, 7, 5, 10, 11, 1, 9, 0, -1, -1, -1, -1, -1, -1, -1},
{10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1, -1, -1, -1, -1},
{11, 1, 2, 11, 7, 1, 7, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11, -1, -1, -1, -1},
{9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7, -1, -1, -1, -1},
{7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2, -1},
{2, 5, 10, 2, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5, -1, -1, -1, -1},
{9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2, -1, -1, -1, -1},
{9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2, -1},
{1, 3, 5, 3, 7, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 7, 0, 7, 1, 1, 7, 5, -1, -1, -1, -1, -1, -1, -1},
{9, 0, 3, 9, 3, 5, 5, 3, 7, -1, -1, -1, -1, -1, -1, -1},
{9, 8, 7, 5, 9, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{5, 8, 4, 5, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0, -1, -1, -1, -1},
{0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5, -1, -1, -1, -1},
{10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4, -1},
{2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8, -1, -1, -1, -1},
{0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11, -1},
{0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5, -1},
{9, 4, 5, 2, 11, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4, -1, -1, -1, -1},
{5, 10, 2, 5, 2, 4, 4, 2, 0, -1, -1, -1, -1, -1, -1, -1},
{3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9, -1},
{5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 3, 5, 1, -1, -1, -1, -1, -1, -1, -1},
{0, 4, 5, 1, 0, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5, -1, -1, -1, -1},
{9, 4, 5, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 11, 7, 4, 9, 11, 9, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11, -1, -1, -1, -1},
{1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11, -1, -1, -1, -1},
{3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4, -1},
{4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2, -1, -1, -1, -1},
{9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3, -1},
{11, 7, 4, 11, 4, 2, 2, 4, 0, -1, -1, -1, -1, -1, -1, -1},
{11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4, -1, -1, -1, -1},
{2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9, -1, -1, -1, -1},
{9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7, -1},
{3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10, -1},
{1, 10, 2, 8, 7, 4, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 7, 1, 3, -1, -1, -1, -1, -1, -1, -1},
{4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1, -1, -1, -1, -1},
{4, 0, 3, 7, 4, 3, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{4, 8, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 8, 10, 11, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 11, 9, 10, -1, -1, -1, -1, -1, -1, -1},
{0, 1, 10, 0, 10, 8, 8, 10, 11, -1, -1, -1, -1, -1, -1, -1},
{3, 1, 10, 11, 3, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 2, 11, 1, 11, 9, 9, 11, 8, -1, -1, -1, -1, -1, -1, -1},
{3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9, -1, -1, -1, -1},
{0, 2, 11, 8, 0, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{3, 2, 11, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 10, 8, 9, -1, -1, -1, -1, -1, -1, -1},
{9, 10, 2, 0, 9, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8, -1, -1, -1, -1},
{1, 10, 2, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{1, 3, 8, 9, 1, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 9, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{0, 3, 8, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1} };
int Polygonise(gridcell grid, float isolevel, int cnt) {
    int i, ntriang;
    int cubeindex;
    xyz vertlist[12];


    cubeindex = 0;
    if (grid.val[0] < isolevel) cubeindex |= 1;
    if (grid.val[1] < isolevel) cubeindex |= 2;
    if (grid.val[2] < isolevel) cubeindex |= 4;
    if (grid.val[3] < isolevel) cubeindex |= 8;
    if (grid.val[4] < isolevel) cubeindex |= 16;
    if (grid.val[5] < isolevel) cubeindex |= 32;
    if (grid.val[6] < isolevel) cubeindex |= 64;
    if (grid.val[7] < isolevel) cubeindex |= 128;
    // Cube is entirely in/out of the surface 
    if (edgeTable[cubeindex] == 0)
        return(0);
    // Find the vertices where the surface intersects the cube 
    if (edgeTable[cubeindex] & 1)
        vertlist[0] = VertexInterp(isolevel, grid.p[0], grid.p[1], grid.val[0], grid.val[1]);
    if (edgeTable[cubeindex] & 2)
        vertlist[1] = VertexInterp(isolevel, grid.p[1], grid.p[2], grid.val[1], grid.val[2]);
    if (edgeTable[cubeindex] & 4)
        vertlist[2] = VertexInterp(isolevel, grid.p[2], grid.p[3], grid.val[2], grid.val[3]);
    if (edgeTable[cubeindex] & 8)
        vertlist[3] = VertexInterp(isolevel, grid.p[3], grid.p[0], grid.val[3], grid.val[0]);
    if (edgeTable[cubeindex] & 16)
        vertlist[4] = VertexInterp(isolevel, grid.p[4], grid.p[5], grid.val[4], grid.val[5]);
    if (edgeTable[cubeindex] & 32)
        vertlist[5] = VertexInterp(isolevel, grid.p[5], grid.p[6], grid.val[5], grid.val[6]);
    if (edgeTable[cubeindex] & 64)
        vertlist[6] = VertexInterp(isolevel, grid.p[6], grid.p[7], grid.val[6], grid.val[7]);
    if (edgeTable[cubeindex] & 128)
        vertlist[7] = VertexInterp(isolevel, grid.p[7], grid.p[4], grid.val[7], grid.val[4]);
    if (edgeTable[cubeindex] & 256)
        vertlist[8] = VertexInterp(isolevel, grid.p[0], grid.p[4], grid.val[0], grid.val[4]);
    if (edgeTable[cubeindex] & 512)
        vertlist[9] = VertexInterp(isolevel, grid.p[1], grid.p[5], grid.val[1], grid.val[5]);
    if (edgeTable[cubeindex] & 1024)
        vertlist[10] = VertexInterp(isolevel, grid.p[2], grid.p[6], grid.val[2], grid.val[6]);
    if (edgeTable[cubeindex] & 2048)
        vertlist[11] = VertexInterp(isolevel, grid.p[3], grid.p[7], grid.val[3], grid.val[7]);
    // Create the triangle 
    ntriang = 0;
    for (i = 0; triTable[cubeindex][i] != -1; i += 3) {
        // Record coord
        cur_tri.p[0] = vertlist[triTable[cubeindex][i]];
        cur_tri.p[1] = vertlist[triTable[cubeindex][i + 1]];
        cur_tri.p[2] = vertlist[triTable[cubeindex][i + 2]];
        // Record edge
        vector<int>edge;
        edge.push_back(triTable[cubeindex][i]);
        edge.push_back(triTable[cubeindex][i + 1]);
        edge.push_back(triTable[cubeindex][i + 2]);
        // Calculate normal
        Calculate_normal1();
        Calculate_normal2(edge, grid);
        triangles[cnt][tri_idx[cnt]] = cur_tri;
        tri_idx[cnt]++;
        ntriang++;
    }
    return(ntriang);
}
void Calculate_histogram() {
    for (int i = 0; i < 255; i++)histogram_data[0][i] = 0;
    for (int i = 0; i < L; i++)
        for (int j = 0; j < W; j++)
            for (int k = 0; k < H; k++)
                histogram_data[0][testing_data[i][j][k]]++;
    for (int i = 0; i < 256; i++)
        histogram_data[1][i] = log(histogram_data[0][i]);
    histogram_data[0][0] = 1;
}

void Draw_triangle(int cnt) {
    if (polygon_mode == GL_FILL) {
        glEnable(GL_LIGHTING);
        float  diffuse[] = { 0.7, 0.7, 0.7, 1.0 };
        float  ambient[] = { 0.3, 0.3, 0.3, 1.0 };
        float  specular[] = { 0.6, 0.6, 0.6, 1.0 };
        float  shininess = 0.9;
        glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
        glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);
        glMaterialfv(GL_FRONT, GL_SPECULAR, specular);
        glMaterialfv(GL_FRONT, GL_DIFFUSE, diffuse);
        glMaterialf(GL_FRONT, GL_SHININESS, shininess);
    }
    else {
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
        glColor3f(triangle_color[cnt][0], triangle_color[cnt][1], triangle_color[cnt][2]);
        glLineWidth(0.1);
    }
    for (int i = 0; i < tri_idx[cnt]; i++) {
        cur_tri = triangles[cnt][i];
        xyz normal[3];
        if (normal_function == 0) {
            normal[0] = xyz(cur_tri.n1.x, cur_tri.n1.y, cur_tri.n1.z);
            normal[1] = xyz(cur_tri.n1.x, cur_tri.n1.y, cur_tri.n1.z);
            normal[2] = xyz(cur_tri.n1.x, cur_tri.n1.y, cur_tri.n1.z);
        }
        else {
            normal[0] = xyz(cur_tri.n2[0].x, cur_tri.n2[0].y, cur_tri.n2[0].z);
            normal[1] = xyz(cur_tri.n2[1].x, cur_tri.n2[1].y, cur_tri.n2[1].z);
            normal[2] = xyz(cur_tri.n2[2].x, cur_tri.n2[2].y, cur_tri.n2[2].z);
        }
        glBegin(GL_TRIANGLES);
        for (int j = 0; j < 3; j++) {
            glNormal3f(normal[j].x, normal[j].y, normal[j].z);
            glVertex3f(cur_tri.p[j].x, cur_tri.p[j].y, cur_tri.p[j].z);
        }
        glEnd();
    }
}
void Draw_triangle_slice(int cnt, int val, int n) {// for slice
    glBegin(GL_POINTS);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
    glColor3f(triangle_color[cnt][0], triangle_color[cnt][1], triangle_color[cnt][2]);
    for (int i = 0; i < tri_idx[cnt]; i++) {
        cur_tri = triangles[cnt][i];
        for (int j = 0; j < 3; j++) {
            if (((n == 0) && ((int)cur_tri.p[j].x == val)) ||
                ((n == 1) && ((int)cur_tri.p[j].y == val)) ||
                ((n == 2) && ((int)cur_tri.p[j].z == val)))
                glVertex3f(cur_tri.p[j].x, cur_tri.p[j].y, cur_tri.p[j].z);
        }
    }
    glEnd();
}
void Read_testing_data_file(int f) {
    cout << "Reading file...\n";
    ifstream file("Scalar\\" + string(filename[f]), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Unable to open file." << std::endl;
        return;
    }
    file.seekg(0, std::ios::end);
    std::streampos fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    vector<unsigned char> buffer(fileSize);
    file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    file.close();
    int index = 0;
    for (int i = 0; i < L; i++)
        for (int j = 0; j < W; j++)
            for (int k = 0; k < H; k++)
                testing_data[i][j][k] = buffer[index++];
}

void Marching_cubes(int cnt) {
    for (int i = 0; i < L - 1; i++) {
        for (int j = 0; j < W - 1; j++) {
            for (int k = 0; k < H - 1; k++) {
                for (int id = 0; id < 8; id++) {
                    int dx = i + offset[id][0];
                    int dy = j + offset[id][1];
                    int dz = k + offset[id][2];
                    grid.val[id] = testing_data[dx][dy][dz];
                    grid.p[id].x = dx;
                    grid.p[id].y = dy;
                    grid.p[id].z = dz;
                }
                int ntriangle = Polygonise(grid, isolevel[file_num][cnt], cnt);
            }
        }
    }
}

void Cross_section() {
    GLdouble clipPlaneNormal[4] = { 0.0, 0.0, 0.0, cross_section_position };
    clipPlaneNormal[cross_section_mode] = -1.0;
    glEnable(GL_CLIP_PLANE0);
    glClipPlane(GL_CLIP_PLANE0, clipPlaneNormal);
}
void Separate_cross_section() { // Slice
    for (int i = 0; i < 3; i++)
        Draw_triangle_slice(i, 100, cross_section_mode);
}

void Draw_light() {
    glPushMatrix();
    glRotatef(light0_angle, 0.0, 0.0, 1.0);
    glTranslatef(200, 200, 0);
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glPopMatrix();
}
void Set_light() {
    light0_angle += 0.5;
    if (light0_angle >= 360)light0_angle -= 360;
    if (light0_on)glEnable(GL_LIGHT0);
    else glDisable(GL_LIGHT0);
    if (light1_on)glEnable(GL_LIGHT1);
    else glDisable(GL_LIGHT1);
    if (light2_on)glEnable(GL_LIGHT2);
    else glDisable(GL_LIGHT2);
    if (light3_on)glEnable(GL_LIGHT3);
    else glDisable(GL_LIGHT3);
}

void draw_rectangle(double r, double g, double b, double xmin, double ymin, double xmax, double ymax) {
    int side = 0;
    vertex[side][0] = xmin, vertex[side][1] = ymin, side++;
    vertex[side][0] = xmax, vertex[side][1] = ymin, side++;
    vertex[side][0] = xmax, vertex[side][1] = ymax, side++;
    vertex[side][0] = xmin, vertex[side][1] = ymax, side++;
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    for (int i = 0; i < 4; i++)
        glVertex2f(vertex[i][0], vertex[i][1]);
    glEnd();
}

void Draw2D() {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT, GL_FILL);

    draw_rectangle(61 / 255.0, 89 / 255.0, 200 / 255.0, 2, 2, 98, 198);// Main panel

    //draw_rectangle(1, 1, 1, 8, 150, 92, 190);

    glColor3f(1.0, 1.0, 1.0);
    int numBins = 90;
    float binWidth = 86.0 / numBins;
    /*
    glBegin(GL_QUADS);
    for (int i = 0; i < numBins; ++i) {
        double x = i * binWidth;
        double y = log10((double)histogram_data[i]);
        glVertex2f(x, 0.0);
        glVertex2f(x + binWidth, 0.0);
        glVertex2f(x + binWidth, y);
        glVertex2f(x, y);
    }
    glEnd();
    */
    glEnable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
}
void Draw3D() {
    Set_light();
    if (cross_slice <= 1) {
        if (cross_slice)Cross_section();
        if (multiple_surface) {
            if (choose_surface == 3) {
                Draw_triangle(0);
                Draw_triangle(1);
                Draw_triangle(2);
            }
            else Draw_triangle(choose_surface);
        }
        else Draw_triangle(0);
    }
    else if (cross_slice == 2)Separate_cross_section();
    glDisable(GL_CLIP_PLANE0);
    Draw_light();
}

void Draw_histogram() {
    ImGui::Begin("Histogram");
    // Choose histogram
    static int selected_histogram = 0;
    ImGui::RadioButton("Origin", &selected_histogram, 0);
    ImGui::SameLine();
    ImGui::RadioButton("Log2", &selected_histogram, 1);

    // Draw histogram
    ImVec4 customColor(1.0f, 1.0f, 1.0f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, customColor);
    ImGui::PlotHistogram("Frequency", histogram_data[selected_histogram], 256, 0, NULL, FLT_MAX, FLT_MAX, ImVec2(200, 150));
    ImGui::PopStyleColor();

    // Draw iso-level line
    for (int i = 0; i < 3; i++) {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetCursorScreenPos();
        float h = ImGui::GetContentRegionAvail().y;
        float y_position = pos.y + h;
        float x_position = pos.x + (isolevel[file_num][i] / 255.0f) * 200 + 1;
        int c[3] = { (int)(triangle_color[i][0] * 255.0), (int)(triangle_color[i][1] * 255.0), (int)(triangle_color[i][2] * 255.0) };
        draw_list->AddLine(ImVec2(x_position, y_position - 160), ImVec2(x_position, y_position - 10), IM_COL32(c[0], c[1], c[2], 255));
        char text_buffer[32];
        sprintf_s(text_buffer, "%d", (int)isolevel[file_num][i]);
        draw_list->AddText(ImVec2(x_position + 5, y_position - 160), IM_COL32(c[0], c[1], c[2], 255), text_buffer);
    }
    ImGui::End();
}
void Draw_control_panel() {
    ImGui::Begin("Control panel");

    // Choose eye spot
    ImGui::Text("Choose eye spot");
    static int selected_eye = 0;
    for (int i = 0; i < 3; i++) {
        ImGui::RadioButton(("Spot " + std::to_string(i)).c_str(), &selected_eye, i);
        ImGui::SameLine();
    }
    ImGui::RadioButton("Customize spot", &selected_eye, 3);
    eye_spot = selected_eye;
    // Customize eye spot
    ImGui::Text("Customize eye spot");
    static int x = eye_coord[3][0], y = eye_coord[3][1], z = eye_coord[3][2];
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("X", &x, -300, 300), ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("Y", &y, -300, 300), ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderInt("Z", &z, -300, 300);
    eye_coord[3][0] = x, eye_coord[3][1] = y, eye_coord[3][2] = z;

    // Choose normal calculation mode
    ImGui::Text("Choose normal calculation method");
    static int selected_normal = 0;
    ImGui::RadioButton("Triangle normal", &selected_normal, 0), ImGui::SameLine();
    ImGui::RadioButton("Gradient", &selected_normal, 1);
    normal_function = selected_normal;

    // Choose polygon mode
    ImGui::Text("Choose polygon mode");
    static int selected_polygon = 0;
    if (polygon_mode == GL_LINES)selected_polygon = 1;
    else selected_polygon = 0;
    ImGui::RadioButton("Fill", &selected_polygon, 0), ImGui::SameLine();
    ImGui::RadioButton("Line", &selected_polygon, 1);
    if (selected_polygon)polygon_mode = GL_LINES;
    else polygon_mode = GL_FILL;

    // Multiple iso-surface
    ImGui::Text("Multiple iso-surface");
    bool enable_multi = multiple_surface;
    ImGui::Checkbox("Open multiple iso-surface", &enable_multi);
    multiple_surface = enable_multi;

    static int selected_surface = choose_surface;
    ImGui::BulletText("Choose multiple iso-surface");
    for (int i = 0; i < 3; i++) {
        ImGui::RadioButton(("Surface " + std::to_string(i + 1)).c_str(), &selected_surface, i);
        ImGui::SameLine();
    }
    ImGui::RadioButton("All", &selected_surface, 3);
    choose_surface = selected_surface;

    // Cross section
    ImGui::Text("Cross section");
    static int selected_cross_slice = cross_slice;
    ImGui::RadioButton("Off", &selected_cross_slice, 0), ImGui::SameLine();
    ImGui::RadioButton("Cross section", &selected_cross_slice, 1), ImGui::SameLine();
    ImGui::RadioButton("Slice", &selected_cross_slice, 2);
    cross_slice = selected_cross_slice;

    static int selected_cross = cross_section_mode;
    ImGui::BulletText("Choose cross section mode");
    ImGui::RadioButton("X direction", &selected_cross, 0), ImGui::SameLine();
    ImGui::RadioButton("Y direction", &selected_cross, 1), ImGui::SameLine();
    ImGui::RadioButton("Z direction", &selected_cross, 2);
    cross_section_mode = selected_cross;

    static float cross_postion = cross_section_position;
    ImGui::SetNextItemWidth(150);
    ImGui::SliderFloat("Position", &cross_postion, 0, 255);
    cross_section_position = cross_postion;

    // Lighting
    ImGui::Text("Lighting");
    bool enable_light0 = light0_on, enable_light1 = light1_on, enable_light2 = light2_on, enable_light3 = light3_on; ;
    ImGui::Checkbox("Light 1", &enable_light0), ImGui::SameLine();
    ImGui::Checkbox("Light 2", &enable_light1), ImGui::SameLine();
    ImGui::Checkbox("Light 3", &enable_light2), ImGui::SameLine();
    ImGui::Checkbox("Light 4", &enable_light3);
    light0_on = enable_light0;
    light1_on = enable_light1;
    light2_on = enable_light2;
    light3_on = enable_light3;

    ImGui::End();
}
void Draw_file_panel() {
    ImGui::Begin("Choose file");
    static int selected_item = 0;
    if (ImGui::BeginCombo("Filename", filename[selected_item], ImGuiComboFlags_None)) {
        for (int i = 0; i < 6; i++) {
            bool is_selected = (selected_item == i);
            if (ImGui::Selectable(filename[i], is_selected)) {
                tri_idx[0] = tri_idx[1] = tri_idx[2] = 0;
                selected_item = i;
                file_num = i;
                Read_testing_data_file(i);
                Calculate_gradient();
                cout << "Marching cubes...\n";
                Marching_cubes(0);
                cout << "Current triangle number: " << tri_idx[0] << "\n";
                Calculate_histogram();
                Marching_cubes(1);
                cout << "Current triangle number: " << tri_idx[1] << "\n";
                Marching_cubes(2);
                cout << "Current triangle number: " << tri_idx[2] << "\n";
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::End();
}
void Display_func() {
    ImGui_ImplOpenGL2_NewFrame();
    ImGui_ImplGLUT_NewFrame();
    ImGui::NewFrame();
    ImGuiIO& io = ImGui::GetIO();
    Draw_histogram();
    Draw_control_panel();
    Draw_file_panel();

    ImGui::ShowDemoWindow();

    // Rendering
    ImGui::Render();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);

    // Draw 3D object
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, width, height);
    gluPerspective(90.0, 1.0, 1.0, 400.0);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(
        eye_coord[eye_spot][0], eye_coord[eye_spot][1], eye_coord[eye_spot][2],
        eye_look[0], eye_look[1], eye_look[2],
        0.0, 1.0, 0.0);
    Draw3D();

    // Draw 2D panel
    /*
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(height, 0, width - height, height);
    gluOrtho2D(0, 100, 0, 200);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Draw2D();
    */

    ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
    glutSwapBuffers(); // Swap the back buffer to the front
    glFlush(); // Display the results
}
// Callback function for idle event
void Idle_func(void) {
    Display_func();
}

void Reshape(int new_w, int new_h) {
    height = new_h, width = new_w;
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();// Record the dimension of the window
    glViewport(0, 0, width, height);
    gluPerspective(90.0, 1.0, 1.0, 400.0);
}

void Init() {
    glClearColor(background_color[0], background_color[1], background_color[2], 1.0); // set the background color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the Depth & Color Buffers
    //glViewport(0, 0, width, height);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_NORMALIZE);
    glShadeModel(GL_SMOOTH);

    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    // Define light0
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
    // Define light1
    glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light0_ambient);
    // Define light2
    glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
    glLightfv(GL_LIGHT2, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT2, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT2, GL_AMBIENT, light0_ambient);
    // Define light3
    glLightfv(GL_LIGHT3, GL_POSITION, light3_position);
    glLightfv(GL_LIGHT3, GL_DIFFUSE, light0_diffuse);
    glLightfv(GL_LIGHT3, GL_SPECULAR, light0_specular);
    glLightfv(GL_LIGHT3, GL_AMBIENT, light0_ambient);

    // Enable face culling
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glFlush(); // Enforce window system display the results
}

void main(int argc, char** argv) {
    srand(time(NULL));

    Read_testing_data_file(0);
    Calculate_gradient();
    cout << "Marching cubes...\n";
    Marching_cubes(0);
    cout << "Current triangle number: " << tri_idx[0] << "\n";
    Calculate_histogram();
    Marching_cubes(1);
    cout << "Current triangle number: " << tri_idx[1] << "\n";
    Marching_cubes(2);
    cout << "Current triangle number: " << tri_idx[2] << "\n";

    glutInit(&argc, argv);
    glutInitWindowPosition(0, 0);
    glutInitWindowSize(width, height); // window size
    glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
    glutCreateWindow("Marching Cubes");

    Init();
    // set background color
    glClearColor(background_color[0], background_color[1], background_color[2], 1.0);
    glClearDepth(1.0);

    // Register callback functions
    glutDisplayFunc(Display_func); // display event
    glutReshapeFunc(Reshape);   // reshape event
    glutIdleFunc(Idle_func);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, polygon_mode);
    glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, 1.0);
    glLightModelf(GL_LIGHT_MODEL_AMBIENT, 1.0);

    Reshape(width, height);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    ImGui_ImplGLUT_Init();
    ImGui_ImplOpenGL2_Init();

    ImGui_ImplGLUT_InstallFuncs();
    glutMainLoop();

    // Cleanup
    ImGui_ImplOpenGL2_Shutdown();
    ImGui_ImplGLUT_Shutdown();
    ImGui::DestroyContext();
}
