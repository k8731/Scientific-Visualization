#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <stdlib.h> 
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <fstream>
#include <ctime>
#include <cmath>
#include <random>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Shader.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glut.h>
#define EPS 1e-8

using namespace std;

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

Shader* shader;
vector<vector<glm::vec3>>w;
vector<glm::vec3>data_vase;
float x_mx = 0, x_mn = 1e18;
float y_mx = 0, y_mn = 1e18;
float z_mx = 0, z_mn = 1e18;
int knode = 32;// lattice: 32*32
int width = 400, height = 800;
int max_iteration = 300000;
float background_color[3] = { 0.0, 0.0, 0.0 };
int n, m;// n-dimension, m data points
vector<float>vertices;
void Read_testing_data_file() {
    // read data file
    cout << "Reading file...\n\n";
    ifstream file("vaseSurface.txt", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Unable to open file." << std::endl;
        return;
    }
    file >> m; // number of sample points
    file >> n; // dimension
    data_vase.resize(m);
    for (int i = 0; i < m; i++) {
        file >> data_vase[i][0] >> data_vase[i][1] >> data_vase[i][2];
        // update max & min
        x_mn = min(x_mn, data_vase[i][0]), x_mx = max(x_mx, data_vase[i][0]);
        y_mn = min(y_mn, data_vase[i][1]), y_mx = max(y_mx, data_vase[i][1]);
        z_mn = min(z_mn, data_vase[i][2]), z_mx = max(z_mx, data_vase[i][2]);
    }
    file.close();
    cout << "Data range:\n";
    cout << "x " << x_mn << " - " << x_mx << '\n';
    cout << "y " << y_mn << " - " << y_mx << '\n';
    cout << "z " << z_mn << " - " << z_mx << "\n\n";
}
glm::vec2 Find_BMU(int p) {// best matching unit(winner)
    double d = 0.0, dmin = 1e18;
    vector<glm::vec2>BMU_nodes;
    for (int i = 0; i < knode; i++) {
        for (int j = 0; j < knode; j++) {
            d = 0.0;
            for (int k = 0; k < 3; k++)
                d += (w[i][j][k] - data_vase[p][k]) * (w[i][j][k] - data_vase[p][k]);
            if (d < dmin) {
                dmin = d;
                BMU_nodes.clear();
                BMU_nodes.push_back(glm::vec2(i, j));
            }
            else if (d == dmin)  BMU_nodes.push_back(glm::vec2(i, j));
        }
    }
    int x = rand() % (int)BMU_nodes.size();
    return BMU_nodes[x];
}
float delta = 0.2;
void Update_nighbor(glm::vec2 bmu, int p, int it) {
    float delta = 0.01 * exp((double) (-it) / max_iteration);// learning rate
    float r = 1.0;
    int x = bmu.x, y = bmu.y;
    
    // update nighborhood weight
    int t = 1;// t layer
    if (it > max_iteration / 2)t = 1;
    else if (it > max_iteration / 5) t = 2;
    else if (it > max_iteration / 10) t = 3;
    else t = 4;
    for (int i = -t + x; i <= t + x; i++) {
        if (i < 0 || i >= knode)continue;
        for (int j = -t + y; j <= t + y; j++) {
            if (j < 0 || j >= knode)continue;
            if (i == x && j == y) {
                // update BMU weight
                w[i][j] += delta * (data_vase[p] - w[x][y]);
            }
            else {
                //sqrt(min((i - x) * (i - x) + (j - y) * (j - y), knode - abs(i - x)) * (knode - abs(i - x)) + (j - y) * (j - y)));
                //int dist = (i - x) * (i - x) + (j - y) * (j - y);
                //int dist = min((i - x) * (i - x) + (j - y) * (j - y), knode - abs(i - x)) * (knode - abs(i - x)) + (j - y) * (j - y);
                int dist = min((i - x) * (i - x) + (j - y) * (j - y), knode - abs(j - y)) * (knode - abs(j - y)) + (i - x) * (i - x);
                if (dist < 1.0) {
                    cout << "x\n";
                }
                r = 1.0 / sqrt(dist);
                w[i][j] += delta * r * (data_vase[p] - w[i][j]);
            }
        }
    }
}
void SOM(int cur_step, int next_step) {
    // iteration
    int it = cur_step;
    while ((it++)< next_step) {
        int p = (int)((double)rand() / RAND_MAX * (m-1));
        glm::vec2 x = Find_BMU(p);
        Update_nighbor(x, p, it);
    }
}
void Init() {
    shader = new Shader("shader.vs", "shader.fs");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Read_testing_data_file();

    // initialize weight
    srand(time(NULL));
    w.resize(knode, vector<glm::vec3>(knode));
    for (int i = 0; i < knode; i++) {
        for (int j = 0; j < knode; j++) {
            /*
            if (j < len) {
                w[i][j].x = x_mn + (float)(x_mx - x_mn) * (j % len) / len;
                w[i][j].y = y_mn;
            }
            else if (j < 2 * len) {
                w[i][j].x = x_mx;
                w[i][j].y = y_mn + (float)(y_mx - y_mn) * (j % len) / len;
            }
            else if (j < 3 * len) {
                w[i][j].x = x_mx - (x_mn + (float)(x_mx - x_mn) * (j % len) / len);
                w[i][j].y = y_mx;
            }
            else {
                w[i][j].x = x_mn;
                w[i][j].y = y_mx - (y_mn + (float)(y_mx - y_mn) * (j % len) / len);
            }
            */
            float theta = (double)j / knode * 360.0 * acos(-1) / 180.0;
            w[i][j].x = 100.0 + 20.0 * acos(-1.0) * sin(theta);
            w[i][j].z = 100.0 + 20.0 * acos(-1.0) * cos(theta);
            w[i][j].y = y_mn + (float)(y_mx - y_mn) * i / knode;
            /*
            w[i][j].x = (int)((double)rand() / (RAND_MAX + 1.0) * (x_mx - x_mn)) + x_mn;
            w[i][j].y = (int)((double)rand() / (RAND_MAX + 1.0) * (y_mx - y_mn)) + y_mn;
            w[i][j].z = (int)((double)rand() / (RAND_MAX + 1.0) * (z_mx - z_mn)) + z_mn;
            */
            //cout << (int)w[i][j].x<< ' ' << (int)w[i][j].y << ' ' << (int)w[i][j].z << '\n';
        }
    }
    cout << '\n';
}

glm::vec3 Add_point(int i, int j) {
    float x, y, z;
    x = 2.0 * (w[i][j].x - x_mn) / (x_mx - x_mn) - 1.0;
    y = 2.0 * (w[i][j].y - y_mn) / (y_mx - y_mn) - 1.0;
    z = 2.0 * (w[i][j].z - z_mn) / (z_mx - z_mn) - 1.0;
    //vertices.push_back(x);
    //vertices.push_back(y);
    //vertices.push_back(z);
    glm::vec3 v(x, y, z);
    return v;
}
glm::vec3 Calculate_normal(glm::vec3 v0, glm::vec3 v1, glm::vec3 v2) {
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));

    vertices.push_back(v0.x);
    vertices.push_back(v0.y);
    vertices.push_back(v0.z);
    vertices.push_back(normal.x);
    vertices.push_back(normal.y);
    vertices.push_back(normal.z);
    vertices.push_back(v1.x);
    vertices.push_back(v1.y);
    vertices.push_back(v1.z);
    vertices.push_back(normal.x);
    vertices.push_back(normal.y);
    vertices.push_back(normal.z);
    vertices.push_back(v2.x);
    vertices.push_back(v2.y);
    vertices.push_back(v2.z);
    vertices.push_back(normal.x);
    vertices.push_back(normal.y);
    vertices.push_back(normal.z);
    return normal;
}
void Configure_vertex_attrib(unsigned int &VBO, unsigned int &VAO) {
    vertices.clear();
    for (int i = 0; i < knode; i++) {
        for (int j = 0; j < knode; j++) {
            /*
            for line
            Add_point(i, j);
            Add_point(i, (j + 1) % knode);// right
            if (i == knode - 1)continue;
            Add_point(i, j);
            Add_point(i + 1, j);// up
            */
            if (i == knode - 1)continue;
            glm::vec3 v0 = Add_point(i, j);
            glm::vec3 v1 = Add_point(i, (j + 1) % knode);
            glm::vec3 v2 = Add_point(i + 1, j);
            glm::vec3 n = Calculate_normal(v0, v1, v2);

            v0 = Add_point(i, (j + 1) % knode);
            v1 = Add_point(i + 1, (j + 1) % knode);
            v2 = Add_point(i + 1, j);
            n = Calculate_normal(v0, v1, v2);
        }
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}
int main(int argc, char** argv) {
    // initialize and configure
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(width, height, "SOM", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    Init();
    //SOM();
    unsigned int VBO, VAO;
    Configure_vertex_attrib(VBO, VAO);
    int tt = 0;
    int cur_step = 0, next_step = 0;
    while (!glfwWindowShouldClose(window)) {
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // Activate shader
        shader->use();

        shader->setVec3("lightPos", glm::vec3(0.0, 0.0, 5.0)); 
        shader->setVec3("lightColor", glm::vec3(1.0, 1.0, 1.0)); 
        shader->setVec3("objectColor", glm::vec3(1.0, 1.0, 1.0));

        tt++;
        float radius = 10.0f;
        float camX = sin(tt / 100.0) * radius;
        float camZ = cos(tt / 100.0) * radius;
        glm::vec3 eye_coord(camX, y_mx / 2.0, camZ);
        //glm::vec3 eye_coord(0,  y_mx / 2.0, 0);
        glm::vec3 eye_look = glm::vec3(0, y_mx / 2.0, 0);
        glm::mat4 projection = glm::perspective(glm::radians(90.0f), (float)width / (float)height, 0.1f, 100.0f);
        glm::mat4 view = glm::lookAt(eye_coord, eye_look, glm::vec3(0.0f, 1.0f, 0.0f));
        shader->setMat4("projection", projection);
        shader->setMat4("view", view);
        
        if (cur_step < max_iteration) {
            next_step = cur_step + max_iteration / 100;
            SOM(cur_step, next_step);
            cur_step = next_step;
            Configure_vertex_attrib(VBO, VAO);
        }

        glDisable(GL_DEPTH_TEST); // disable for draw lines!
        // Render
        //glLineWidth(2.0f);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, (int)vertices.size() / 6);
        //glDrawArrays(GL_TRIANGLES, 0, (int)vertices.size() / 3);

        glEnable(GL_DEPTH_TEST); 

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader->ID);

    glfwTerminate();

    return 0;
}
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}