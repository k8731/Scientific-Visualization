#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <stdlib.h> 
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "Shader.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glut.h>
#include "implot.h"
#include "implot_internal.h"

using namespace std;
#define EPS 1e-12

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

Shader* shader;

// Data and gradient parameter
const int L = 256, W = 256, H = 256;
unsigned char testing_data[L][W][H];
glm::vec3 gradient[L][W][H];
// Draw parameter
float width = 1080.0;
float height = 720.0;
GLenum  polygon_mode = GL_FILL;
float background_color[3] = { 10 / 255.0 ,10 / 255.0 ,70 / 255.0 };
// Eye parameter
glm::vec3 eye_coord[4] = {
    glm::vec3(250, 0, 0),
    glm::vec3(0, 250, 0),
    glm::vec3(0, 0, 250),
    glm::vec3(252, 152, 272)
    //glm::vec3(127, 127, 300)
};
glm::vec3 eye_look = glm::vec3(128, 128, 128);
// Projection parameter
int projection_mode = 0; // 0:ortho 1:persp
int eye_spot = 3;
// File parameter
int file_num = 0;
const char* filename[6] = { "testing_engine.raw", "foot.raw", "Carp.raw", "bonsai.raw", "aneurism.raw" , "skull.raw" };
// Texture parameter
unsigned int texName3D, texName1D;
// Vertices(for shader) parameter
float vertices[12 * 3 * 6];// 12 triangle, attrib(world coord3+texture coord3)
// Histogram parameter
float histogram_data[2][256] = { 0 }; // 0:origin 1:log2
// Gap parameter 
float gap_t = 0.1;
// Phong effect parameter
bool Phong = false;
// Transfer function parameter
struct trans_node {
    int s, t;// start to finish
    int id;// node id
    glm::vec4 color;
    trans_node() { s = 0, t = 0, color = glm::vec4(0), id = 0; }
    trans_node(int s_, int t_, glm::vec4 c, int id_) {
        s = s_, t = t_, color = c, id = id_;
    }
};
int trans[4][256]={0};
vector<trans_node>trans_section;
float transfer_function[256] = { 0 }; // for draw
void Calculate_gradient() {
    glm::vec3 grad;
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < W; j++) {
            for (int k = 0; k < H; k++) {
                if (i == 0)grad.x = testing_data[i + 1][j][k] - testing_data[i][j][k];
                else if (i == L - 1)grad.x = testing_data[i][j][k] - testing_data[i - 1][j][k];
                else grad.x = (testing_data[i + 1][j][k] - testing_data[i - 1][j][k])/2;
                if (j == 0)grad.y = testing_data[i][j + 1][k] - testing_data[i][j][k];
                else if (j == W - 1)grad.y = testing_data[i][j][k] - testing_data[i][j - 1][k];
                else grad.y = (testing_data[i][j + 1][k] - testing_data[i][j - 1][k])/2;
                if (k == 0)grad.z = testing_data[i][j][k + 1] - testing_data[i][j][k];
                else if (k == H - 1) grad.z = testing_data[i][j][k] - testing_data[i][j][k - 1];
                else grad.z = (testing_data[i][j][k + 1] - testing_data[i][j][k - 1])/2;
                gradient[i][j][k] = normalize(grad);
            }
        }
    }
}
void Read_testing_data_file(int f) {
    cout << "Reading file...\n";
    ifstream file("source\\" + string(filename[f]), std::ios::binary);
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
void Create_texture3D() {
    // create texture value
    Calculate_gradient();
    unsigned char(*texture3D)[4] = new unsigned char[L * W * H][4];
    for (int k = 0; k < H; k++) {
        for (int j = 0; j < W; j++) {
            for (int i = 0; i < L; i++) {
                int index = k * L * W + j * L + i;
                glm::vec3 g = gradient[i][j][k];
                g = glm::normalize(g);
                texture3D[index][0] = (g.x + 1) / 2 * 255;
                texture3D[index][1] = (g.y + 1) / 2 * 255;
                texture3D[index][2] = (g.z + 1) / 2 * 255;
                texture3D[index][3] = testing_data[i][j][k];
            }
        }
    }
    // Generate texture obj IDs
    glGenTextures(1, &texName3D);
    // Bind the 3d texture
    glBindTexture(GL_TEXTURE_3D, texName3D);
    // Define min. and mag filters
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Set wrapping strategies to repeating
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // Set unpack alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Fill in texel data
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA, L, W, H, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture3D);
}
void Create_texture1D() {
    // create texture value
    unsigned char(*texture1D)[4] = new unsigned char[256][4];
    for (int i = 0; i < 256; i++) {
        for (int j = 0; j < 4; j++)
            texture1D[i][j] = trans[j][i];
        /*
        if (i < 25) {
            texture1D[i][0] = 0;
            texture1D[i][1] = 0;
            texture1D[i][2] = 0;
            texture1D[i][3] = 0;
        }
        else if (i < 50) {
            texture1D[i][0] = 255;
            texture1D[i][1] = 0;
            texture1D[i][2] = 0;
            texture1D[i][3] = 1;
        }
        else if (i < 100) {
            texture1D[i][0] = 0;
            texture1D[i][1] = 255;
            texture1D[i][2] = 0;
            texture1D[i][3] = 1;
        }
        else if (i < 150) {
            texture1D[i][0] = 0;
            texture1D[i][1] = 0;
            texture1D[i][2] = 255;
            texture1D[i][3] = 1;
        }
        else {
            texture1D[i][0] = 0;
            texture1D[i][1] = 255;
            texture1D[i][2] = 255;
            texture1D[i][3] = 1;
        }
        */
        
    }
    // Generate texture obj IDs
    glGenTextures(1, &texName1D);
    // Bind the 1d texture
    glBindTexture(GL_TEXTURE_1D, texName1D);
    // Define min. and mag filters
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Set wrapping strategies to repeating
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    // Set unpack alignment
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Fill in texel data
    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, texture1D);
}
void Bind_textures() {
    // Set texture
    shader->setInt("texture1D", 1);
    shader->setInt("texture3D", 0);

    // Bind textures on corresponding texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_3D, texName3D);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, texName1D);
}
void Create_transformations() {
    // Create transformations and set ray direction
    glm::mat4 projection;
    if (projection_mode) {// perspective
        projection = glm::perspective(glm::radians(90.0f), width / height, -100.0f, 1000.0f);
        shader->setVec3("eye", eye_coord[eye_spot]);
    }
    else {// ortho
        projection = glm::ortho(-200.0f, 200.0f, -150.0f, 200.0f, -100.0f, 1000.0f);
        glm::vec3 d = glm::vec3(eye_look - eye_coord[eye_spot]);
        d = glm::normalize(d);
        shader->setVec3("d", d);
    }
    glm::mat4 view = glm::lookAt(eye_coord[eye_spot], eye_look, glm::vec3(0.0f, 1.0f, 0.0f));
    shader->setInt("projection_mode", projection_mode);
    shader->setMat4("projection", projection);
    shader->setMat4("view", view);
}
void Calculate_vertice_attrib() {
    unsigned const int offset[8][3] = {
        {0, 0, 0},{1, 0, 0},{1, 1, 0},{0, 1, 0},
        {0, 0, 1},{1, 0, 1},{1, 1, 1},{0, 1, 1}
    };
    unsigned int indices[] = {// 12 triangle
        0, 1, 3, 1, 2, 3, // 2 triangle
        1, 5, 2, 5, 6, 2,
        3, 2, 7, 2, 6, 7,
        4, 0, 7, 0, 3, 7,
        5, 4, 6, 4, 7, 6,
        4, 5, 0, 5, 1, 0
    };
    for (int i = 0; i < 12; i++) {// triangle
        for (int j = 0; j < 3; j++) {// 3 vertice
            int vb = indices[i * 3 + j];// offset[vb]:(x,y,z)
            int ab = (i * 3 + j) * 6;
            for (int k = 0; k < 3; k++) {// x y z
                vertices[ab + k] = offset[vb][k] * 255; // world coord
                vertices[ab + k + 3] = offset[vb][k];   // texture coord
            }
        }
    }
}
void Configure_vertex_attrib(unsigned int& VBO, unsigned int& VAO) {
    // Set up vertex data (and buffer(s)) and configure vertex attributes
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // configure vertex attributes
    // world coord
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // texture coord
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
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
void Init() {
    shader = new Shader("shader.vs", "shader.fs");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Read_testing_data_file(0);
    Calculate_histogram();
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

    ImGui::End();
}
void Update_transfer_function() {
    trans_node node;
    glm::vec4 c;
    for (int i = 0; i < 256; i++)
        for (int j = 0; j < 4; j++)trans[j][i] = 0;
    for (int i = 0; i < (int)trans_section.size(); i++) {
        node = trans_section[i];
        c = node.color;
        for (int j = node.s; j <= node.t; j++) {
            trans[0][j] = c.x;// r
            trans[1][j] = c.y;// g
            trans[2][j] = c.z;// b
            trans[3][j] = c.w;// a
        }
    }
    Create_texture1D();
}
void Delete_transfer_section() {
    trans_section.pop_back();
}
void Draw_transfer_function() {
    ImGui::Begin("Transfer function");
    // Input color section value
    ImGui::Text("Type in color section values");
    static char s[5], t[5], r[5], g[5], b[5], a[5];
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("Start", s, 5), ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::InputText("End", t, 5);
    ImGui::SetNextItemWidth(50);
    ImGui::InputText("R", r, 5), ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::InputText("G", g, 5), ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::InputText("B", b, 5), ImGui::SameLine();
    ImGui::SetNextItemWidth(50);
    ImGui::InputText("A", a, 5);

    // Add color section
    if (ImGui::Button("Add color section")){
        int ss = atoi(s), tt = atoi(t), rr = atoi(r), gg = atoi(g), bb = atoi(b), aa = atoi(a);
        int id = (int)trans_section.size() + 1;
        trans_node node(ss, tt, glm::vec4(rr, gg, bb, aa), id);
        trans_section.push_back(node);
        Update_transfer_function();
    }
    int xtemp[256];
    for (int i = 0; i < 256; i++)xtemp[i] = i;
    ImPlot::BeginPlot(" ");
    ImPlot::SetupAxesLimits(0, 256, 0, 256, 10);
    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
    ImPlot::PlotLine("R", xtemp, trans[0], 256);
    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    ImPlot::PlotLine("G", xtemp, trans[1], 256);
    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
    ImPlot::PlotLine("B", xtemp, trans[2], 256);
    ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImPlot::PlotLine("A", xtemp, trans[3], 256);
    ImPlot::EndPlot();
    /*
    int xtemp[256];
    for (int i = 0; i < 256; i++)xtemp[i] = i;
    ImPlot::BeginPlot(" ");
    ImPlot::SetupAxesLimits(0, 256, 0, 256, 10);
    for (int i = 0; i < (int)trans_section.size(); i++) {
        int ytemp[256][4]={0};
        trans_node node = trans_section[i];
        glm::vec4 c = node.color;
        for (int j = node.s; j <= node.t; j++) {
            ytemp[0][j] = c.x;
            ytemp[1][j] = c.y;
            ytemp[2][j] = c.z;
            ytemp[3][j] = c.w;
        }
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(c.x, 0, 0, c.w));
        ImPlot::PlotLine("R", xtemp, ytemp[0], 256);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0, c.y, 0, c.w));
        ImPlot::PlotLine("G", xtemp, ytemp[1], 256);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(0, 0, c.z, c.w));
        ImPlot::PlotLine("B", xtemp, ytemp[2], 256);
        ImPlot::PushStyleColor(ImPlotCol_Line, ImVec4(255, 255, 255, c.w));
        ImPlot::PlotLine("A", xtemp, ytemp[3], 256);
    }
    ImPlot::EndPlot();
    */
    // Show section info
    ImGui::Text("Current color section:");
    for (int i = 0; i < (int)trans_section.size(); i++) {
        trans_node node = trans_section[i];
        glm::vec4 c = node.color;
        string s = to_string(i) + "  R:" + to_string((int)c.x);
        s = s + ", G:" + to_string((int)c.y);
        s = s + ", B:" + to_string((int)c.z);
        s = s + ", A:" + to_string((int)c.a);
        ImGui::BulletText(s.c_str());
    }
    if (ImGui::Button("Delete the last section")) {
        if ((int)trans_section.size() != 0) {
            Delete_transfer_section();
            Update_transfer_function();
        }
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

    // Choose projection mode
    ImGui::Text("Choose projection mode");
    static int selected_projection = projection_mode;
    ImGui::RadioButton("Orthographic", &selected_projection, 0), ImGui::SameLine();
    ImGui::RadioButton("Perspective", &selected_projection, 1);
    projection_mode = selected_projection;

    // Set gap
    ImGui::Text("Choose gap");
    static float gap = gap_t;
    ImGui::SetNextItemWidth(320);
    ImGui::SliderFloat("Gap", &gap, 0.04, 0.2);
    gap_t = gap;
    
    // Use Phong effect
    ImGui::Text("Use Phong");
    static bool phong = Phong;
    ImGui::Checkbox("Use Phong", &phong);
    Phong = phong;

    ImGui::End();
}
void Draw_file_panel() {
    ImGui::Begin("Choose file");
    static int selected_item = 0;
    if (ImGui::BeginCombo("Filename", filename[selected_item], ImGuiComboFlags_None)) {
        for (int i = 0; i < 6; i++) {
            bool is_selected = (selected_item == i);
            if (ImGui::Selectable(filename[i], is_selected)) {
                trans_section.clear();
                for (int j = 0; j < 256; j++)for(int k=0;k<4;k++)trans[k][i] = 0;
                Update_transfer_function();
                Read_testing_data_file(i);
                Calculate_histogram();

                Calculate_vertice_attrib();
                Create_texture1D();
                Create_texture3D();
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    ImGui::End();
}
int main(int argc, char** argv) {
    // initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    GLFWwindow* window = glfwCreateWindow(width, height, "Ray Casting", NULL, NULL);
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

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);   
    ImGui_ImplOpenGL3_Init();

    Init();

    Calculate_vertice_attrib();
    Create_texture1D();
    Create_texture3D();

    unsigned int VBO, VAO;
    Configure_vertex_attrib(VBO, VAO);


    // Render loop
    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();
        Draw_histogram();
        Draw_control_panel();
        Draw_file_panel();
        Draw_transfer_function();

        ImGui::ShowDemoWindow();

        // Rendering
        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(background_color[0], background_color[1], background_color[2], 1.0);
        
        // Activate shader
        shader->use();

        shader->setFloat("t", gap_t);// set gap
        shader->setVec3("lightPos", eye_coord[eye_spot]);     
        shader->setBool("Phong", Phong);
        Create_transformations();
        Bind_textures();

        // Render box
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shader->ID);
    glfwTerminate();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
}
// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}