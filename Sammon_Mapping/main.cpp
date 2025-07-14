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

vector<vector<double>>data_card, dist;// dist:origin
vector<glm::vec2> Q;// 2-D space
int L = 1200, W = 1200;
float background_color[3] = { 1.0, 1.0, 1.0 };
int n, m;// n-dimension, m data points
int num = 400; // sample points number
set<int>choose_s; // random choose sample points
//vector<int>choose;
vector<float>vertices;
unsigned int VBO, VAO;
void Kohonen(){
    double a = 0.9;
    double b = 0.4;
    int s;
    for (s = 0;;s++) {
        //cout << s << " iteration\n";
        for (int i = 0; i < num; i++) {
            for (int j = 0; j < num; j++) {
                if (i == j)continue;
                double d = (Q[i].x - Q[j].x) * (Q[i].x - Q[j].x) + (Q[i].y - Q[j].y) * (Q[i].y - Q[j].y);// new
                //if (d == 0.0)d = EPS;
                if (d < EPS)break;
                double dis = dist[i][j] / sqrt(n - 1);
                float diff = b * (dis - d) / d;
                //cout << delta << '\n';
                glm::vec2 di = diff * glm::vec2(Q[i] - Q[j]);
                glm::vec2 dj = -di;
                Q[i] = Q[i] + di;
                Q[j] = Q[j] + dj;
            }
        }
        b = b * a;
        if (b < EPS)
            break;
    }
    cout << s << " iteration\n";
    /*
    cout << "2-D coord:\n";
    for (int i = 0; i < num; i++) {
        cout << i << ": ";
        cout << Q[i].x << ' ' << Q[i].y << '\n';
    }*/
}
void Read_testing_data_file() {
    // read data file
    cout << "Reading file...\n";
    ifstream file("creditcard.dat", std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Unable to open file." << std::endl;
        return;
    }
    file >> m;
    file.ignore(1, ',');
    file >> n;
    data_card.resize(m, vector<double>(n,0));
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            file >> data_card[i][j];
            file.ignore(1, ',');
        }
    }
    file.close();
    // choose random sample points
    cout << "Random choose sample points:";
    srand((unsigned)time(NULL));
    /*
    while (choose_s.size() < num) {
        int num = rand() % m;
        choose_s.insert(num);
    }
    for (const auto& s : choose_s) {
        cout << " " << s;
        choose.push_back(s);
    }
    */
    cout << '\n';
}
void Init() {
    shader = new Shader("shader.vs", "shader.fs");
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Read_testing_data_file();
    // Calculation origin distance
    dist.resize(num, vector<double>(num, 0));
    for (int i = 0; i < num; i++) {
        //int x = choose[i];
        int x = i;
        for (int j = 0; j < num; j++) {
            if (j == i)continue;
            //int y = choose[j];
            int y = j;
            float d = 0.0;
            for (int k = 0; k < n - 1; k++) // 0~n-2 attribute
                d += (data_card[x][k] - data_card[y][k]) * (data_card[x][k] - data_card[y][k]);
            dist[i][j] = d;
        }
    }
    // Initialize Q coord
    Q.resize(num);
    for (int i = 0; i < num; i++) {// 0~1
        Q[i].x = (double)rand() / (RAND_MAX + 1.0);
        Q[i].y = (double)rand() / (RAND_MAX + 1.0);
    }
}
void Configure_vertex_attrib() {
    double x = 5;
    for (int i = 0; i < num; i++) {
        // make circle
        for (double ii = -x/L; ii < (x + 1)/L; ii+=2.0/L) {
            for (double jj = -x/W; jj < (x + 1)/W; jj+=2.0/W) {
                double ti = Q[i].x + ii, tj = Q[i].y + jj;
                if (ii * ii + jj * jj > x * x)continue; // circle
                vertices.push_back(ti - 0.5);
                vertices.push_back(tj - 0.5);
                if (data_card[i][n - 1] == 1.0) { // red
                    vertices.push_back(1.0);
                    vertices.push_back(0.0);
                    vertices.push_back(0.0);
                }
                else { // blue
                    vertices.push_back(0.0);
                    vertices.push_back(0.0);
                    vertices.push_back(1.0);
                }
            }
        }
    }
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}
int main(int argc, char** argv) {
    
    // initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(L, W, "Summon Mapping", NULL, NULL);
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
    Kohonen();
    Configure_vertex_attrib();

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(background_color[0], background_color[1], background_color[2], 1.0);

        // Activate shader
        shader->use();

        // Render
        glBindVertexArray(VAO);
        glDrawArrays(GL_POINTS, 0, (int)vertices.size()/5);
        glBindVertexArray(0); // unbind VAO
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader->ID);
    glfwTerminate();
}
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, L, W);
}