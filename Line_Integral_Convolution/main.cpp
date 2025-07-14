#define _CRT_SECURE_NO_WARNINGS
#include <math.h>
#include <stdio.h>
#include <stdlib.h> 
#include <iostream>
#include <string>
#include <sstream>
#include <vector>
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
#define EPS 1e-6

using namespace std;

const float fac = sqrt((2.0) * acos(-1));

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);

Shader* shader;

struct node {
    glm::vec2 p, v;
    glm::vec3 c;// color
    glm::vec3 cg;// color(gray)
    node() {
        p = glm::vec2(0.f, 0.f);
        v = glm::vec2(0.f, 0.f);
        c = glm::vec3(0.f, 0.f, 0.f);
        cg = glm::vec3(0.f, 0.f, 0.f);
    }
    node(glm::vec2 p_, glm::vec2 v_, glm::vec3 c_, glm::vec3 cg_) {
        p = p_, v = v_, c = c_, cg = cg_;
    }
};
node tmp_node;
// Data parameter
int L, W;// vector field domain resolution
vector<vector<vector<float>>>vector_field;
// Draw parameter
// image size = vector field domain resolution * mult_factor
int L_screen = 1400, W_screen = 700; 
int L_image, W_image;
int mult_factor = 10;
int point_cnt = 0;
vector<vector<float>>noise;// L_image * W_image
vector<vector<bool>>flag;// L_image * W_image
GLenum  polygon_mode = GL_FILL;
float background_color[3] = { 1.0, 1.0, 1.0 };
// Noise setting parameter
int noise_type = 0;
int convolution_type = 0;
int convolution_mask = 5;
// Calculation parameter
vector<vector<node>>streamlines;
vector<node>streamline;
double mx, mn; // max speed & min speed
// Color parameter
bool show_color = false;
// File parameter
int file_num = 0;
const char* filename[21] = {
    "1.vec", "2.vec", "3.vec", "4.vec", "5.vec", "6.vec", "7.vec",
    "8.vec", "9.vec", "10.vec", "11.vec", "12.vec", "13.vec", "14.vec",
    "15.vec", "16.vec", "19.vec", "20.vec", "21.vec", "22.vec", "23.vec"
};
bool state_file = false; // change file state
// VAO VBO
unsigned int VBO_image, VAO_image; // for image
unsigned int VBO_noise, VAO_noise; // for noise
glm::vec2 Calculate_velocity(glm::vec2 p) {
    // [dx,dy]: vector field space
    int dx = (int)p.x / mult_factor, dy = (int)p.y / mult_factor;
    if ((dx + 1 >= L) || (dy + 1 >= W))
        return glm::vec2(vector_field[dx][dy][0], vector_field[dx][dy][1]);
    // interpolation
    glm::vec2 v;
    glm::vec2 vmn(vector_field[dx][dy][0], vector_field[dx][dy][1]);
    glm::vec2 vmx(vector_field[dx + 1][dy + 1][0], vector_field[dx + 1][dy + 1][1]);
    v.x = vmn.x + (vmx.x - vmn.x) * (p.x - (int)p.x);
    v.y = vmn.y + (vmx.y - vmn.y) * (p.y - (int)p.y);
    return v;
}
bool Out(glm::vec2 p) {
    if (p.x >= L_image || p.x < 0 || p.y >= W_image || p.y < 0)return true;
    if (flag[(int)p.x][(int)p.y])return true;
    return false;
}
glm::vec3 Calculate_color(double ratio) {
    glm::vec3 color;
    if (ratio < 0.5f) { // red -> green
        ratio *= 2; // 0 to 1
        color.r = 1.0f - ratio;
        color.g = ratio;
        color.b = 0.0f;
    }
    else { // green -> blue
        ratio = (ratio - 0.5f) * 2; // 0 -> 1
        color.r = 0.0f;
        color.g = 1.0f - ratio;
        color.b = ratio;
    }
    return color;
}
void Convolution(int id) {// id:index of streamlines
    float sigma = 7.0;
    float ratio;
    for (int ii = 0; ii < (int)streamlines[id].size(); ii++) {
        flag[(int)streamlines[id][ii].p.x][(int)streamlines[id][ii].p.y] = 1;
        float cnt = 0, sum = 0;
        for (int k = -convolution_mask; k < convolution_mask; k++) {
            if (ii + k >= (int)streamlines[id].size() || ii + k < 0)continue;
            if (convolution_type == 0) // Gaussian
                ratio = exp(-k * k / (2.0 * sigma * sigma)) / (fac * sigma);
            else if (convolution_type == 1) // box filter
                ratio = 1.0;
            else // tent filter
                ratio = (float)abs(k) / (float)convolution_mask;
            cnt += ratio;
            sum += noise[(int)streamlines[id][ii + k].p.x][(int)streamlines[id][ii + k].p.y] * ratio;
        }
        sum /= cnt;
        glm::vec2 v = streamlines[id][ii].v;
        double ratio= ((v.x * v.x + v.y * v.y) - mn) / (mx - mn);
        streamlines[id][ii].c = Calculate_color(ratio) * sum;
        streamlines[id][ii].cg = glm::vec3(sum, sum, sum);
    }
}
void RK2() {
    flag.resize(L_image, vector<bool>(W_image, false));
    float h = 5.0;
    int di, dj;
    glm::vec2 p1, v1, p, v;
    glm::vec3 c_t(0.f, 0.f, 0.f);
    for (int i = 0; i < L_image; i+=1) {// [i,j]: image space
        for (int j = 0; j < W_image; j+=1) {
            if (flag[i][j])continue;
            di = i / mult_factor, dj = j / mult_factor;// [di,dj]: vector field space
            glm::vec2 p0((float)i, (float)j);
            glm::vec2 v0(vector_field[di][dj][0], vector_field[di][dj][1]);
            streamline.push_back(node(p0, v0, c_t, c_t));
            while (true) {
                float sq = v0.x * v0.x + v0.y * v0.y;
                if (sq < 1e-5)break;
                h = 1.0/(sqrt(sq)+0.1); // Adaptive h
                p1 = p0 + v0* h;

                if (Out(p1))break;
                v1 = Calculate_velocity(p1);
                v = (v0 + v1) / 2.0f;
                p = p0 + h * v;
                if (Out(p))break;
                glm::vec2 p_((int)p.x, (int)p.y);
                streamline.push_back(node(p_, v, c_t, c_t));
                //streamline.push_back(node(p, v, c_t, c_t));
                if ((int)streamline.size() > 3000000)break; // too long, break
                p0 = p, v0 = v;
            }
            /*
            int n = (int)streamline.size();
            if (n >= 4 && n < 7) {
                int n = (int)streamline.size();
                node last1 = streamline[n-1];
                node last2 = streamline[n-2];
                node fake;
                glm::vec2 fp;
                fp.x = (last2.p.x - last1.p.x) + last2.p.x;
                fp.y = (last2.p.y - last1.p.y) + last2.p.y;
                if (!Out(fp)) {
                    last2.p = fp;
                    streamline.push_back(last2);
                }
                //node fake(last2.p.x- last2.p.y)
            }
            else if (n < 4) {
                streamline.clear();
                continue;
            }
            */
            if ((int)streamline.size() <= 7) { // too short, skip
                streamline.clear();
                continue;
            }
            
            streamlines.push_back(streamline);
            streamline.clear();
            Convolution((int)streamlines.size() - 1);
        }
    }
}
void Create_noise_texture_low_frequency() {
    float sigma = 5;
    if (noise.empty())noise.resize(L_image, vector<float>(W_image, 0.0f));
    // Create random noise
    for (int i = 0; i < L_image; i++)
        for (int j = 0; j < W_image; j++)
            noise[i][j] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    vector<vector<float>>tmp = noise;
    // Blur noise
    int x = 1; // mask size
    for (int i = 0; i < L_image; i++) {
        for (int j = 0; j < W_image; j++) {
            float cnt = 0, sum = 0;
            for (int ii = -x; ii < x + 1; ii++) {
                for (int jj = -x; jj < x + 1; jj++) {
                    int ti = i + ii, tj = j + jj;
                    if (ti < 0 || ti >= L_image || tj < 0 || tj >= W_image)continue;
                    cnt += 1.0;
                    sum += tmp[ti][tj];
                }
            }
            noise[i][j] = sum / cnt;
        }
    }
}
void Create_noise_texture_gray() {
    float sigma = 5;
    if (noise.empty())noise.resize(L_image, vector<float>(W_image, 0.0f));
    // Create random noise
    for (int i = 0; i < L_image; i++)
        for (int j = 0; j < W_image; j++)
            noise[i][j] = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);
    vector<vector<float>>tmp = noise;
}
void Create_noise_texture_white_black() {
    srand((unsigned)time(NULL));
    if (noise.empty())noise.resize(L_image, vector<float>(W_image, 0.0f));
    // Create random noise
    for (int i = 0; i < L_image; i++) {
        for (int j = 0; j < W_image; j++) {
            int idx = rand() % W_image;
            noise[i][idx] = rand() % 2;
        }
    }
}
void Create_noise_texture_spot() {
    srand((unsigned)time(NULL));
    if (noise.empty())noise.resize(L_image, vector<float>(W_image, 0.0f));
    else {
        for (int i = 0; i < L_image; i++)
            for (int j = 0; j < W_image; j++)
                noise[i][j] = 0.0;
    }
    float sigma = 0.8;
    int spot_num = rand() % 200000 + 100000;
    for (int i = 0; i < spot_num; i++) {
        int ri = rand() % (L_image - 1), rj = rand() % (W_image - 1);
        if (noise[ri][rj] > 0)continue;
        float c = static_cast <float> (rand()) / static_cast <float> (RAND_MAX);;
        int x = rand() % 7;
        for (int ii = -x; ii < x + 1; ii++) {
            for (int jj = -x; jj < x + 1; jj++) {
                float ti = ri + ii, tj = rj + jj;
                if (ti < 0 || ti >= L_image || tj < 0 || tj >= W_image)continue;
                if (ii * ii + jj * jj > x * x)continue; // circle
                noise[ti][tj] = c;
            }
        }
    }
}
void Read_testing_data_file(int f) {
    cout << "Reading file: " << filename[f] << "\n";
    ifstream file("Vector\\" + string(filename[f]), std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Unable to open file." << std::endl;
        return;
    }
    if (!(file >> L >> W)) {
        std::cerr << "Error reading integers from file.\n";
        return;
    }
    mn = 1e12, mx = 0;
    vector_field.resize(L, vector<vector<float>>(W, vector<float>(2, 0.0f)));
    for (int i = 0; i < L; i++) {
        for (int j = 0; j < W; j++) {
            for (int k = 0; k < 2; k++)
                file >> vector_field[i][j][k];
            double s = vector_field[i][j][0] * vector_field[i][j][0] + vector_field[i][j][1] * vector_field[i][j][1];
            mn = min(mn, s), mx = max(mx, s);
        }
    }
    file.close();
}
void Configure_image_vertex_attrib() {
    // Save image vertices
    vector<float>image_vertices;
    for (int i = 0; i < (int)streamlines.size(); i++) {
        for (int j = 0; j < (int)streamlines[i].size() - 1; j++) {
            for (int k = 0; k < 2; k++) {// add 2 node in one line
                tmp_node = streamlines[i][j + k];
                image_vertices.push_back(tmp_node.p.x / (L_image - 1.0));
                image_vertices.push_back(tmp_node.p.y / (W_image - 1.0));
                if (show_color) {
                    image_vertices.push_back(tmp_node.c.r);
                    image_vertices.push_back(tmp_node.c.g);
                    image_vertices.push_back(tmp_node.c.b);
                }
                else {// gray level
                    image_vertices.push_back(tmp_node.cg.r);
                    image_vertices.push_back(tmp_node.cg.g);
                    image_vertices.push_back(tmp_node.cg.b);
                }
            }
        }
    }
    point_cnt = (int)image_vertices.size() / 5;
    // Convert to NDC coord
    for (int i = 0; i < (int)image_vertices.size(); i += 5) {
        image_vertices[i] = image_vertices[i] * -1.0f;              // x:-1~0
        image_vertices[i+1] = image_vertices[i + 1] * 2.0f - 1.0f;  // y:-1~1
    }
    // Set up vertex data and buffer
    glGenVertexArrays(1, &VAO_image);
    glGenBuffers(1, &VBO_image);
    glBindVertexArray(VAO_image);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_image);
    glBufferData(GL_ARRAY_BUFFER, image_vertices.size() * sizeof(float), image_vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}
void Configure_noise_vertex_attrib() {
    // Save noise vertices
    vector<float>noise_vertices;
    for (int i = 0; i < L_image; i++) {
        for (int j = 0; j < W_image; j++) {
            noise_vertices.push_back(i / (L_image - 1.0));
            noise_vertices.push_back(j / (W_image - 1.0));
            noise_vertices.push_back(noise[i][j]);// r
            noise_vertices.push_back(noise[i][j]);// g
            noise_vertices.push_back(noise[i][j]);// b
        }
    }
    // Convert to NDC coord
    for (int i = 0; i < (int)noise_vertices.size(); i += 5) {
        noise_vertices[i] = noise_vertices[i] * 1.0f;               // x:0~1
        noise_vertices[i + 1] = noise_vertices[i + 1] * 2.0f - 1.0f;// y:-1~1
    }
    // Set up vertex data and buffer
    glGenVertexArrays(1, &VAO_noise);
    glGenBuffers(1, &VBO_noise);
    glBindVertexArray(VAO_noise);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_noise);
    glBufferData(GL_ARRAY_BUFFER, noise_vertices.size() * sizeof(float), noise_vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
}
void Init() {
    shader = new Shader("shader.vs", "shader.fs");
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    Read_testing_data_file(0);
    L_image = L * mult_factor, W_image = W * mult_factor;
    L_screen = L_image * 2, W_screen = W_image;
}
void Update_convolution() {
    for (int i = 0; i < (int)streamlines.size(); i++)Convolution(i);
    Configure_image_vertex_attrib();
}
void Update_noise() {
    if (noise_type == 0)Create_noise_texture_low_frequency();
    else if (noise_type == 1)Create_noise_texture_gray();
    else if (noise_type == 2)Create_noise_texture_white_black();
    else Create_noise_texture_spot();
    Update_convolution();
    Configure_noise_vertex_attrib();
}
void Draw_control_panel() {
    ImGui::Begin("Control panel");
    // Choose noise type
    ImGui::Text("Choose noise type");
    static int selected_noise_type = noise_type;
    if (ImGui::RadioButton("Low frequency noise", &selected_noise_type, 0)) {
        if (selected_noise_type != noise_type) {
            noise_type = selected_noise_type;
            Update_noise();
        }
    }
    if (ImGui::RadioButton("High frequency noise(gray)", &selected_noise_type, 1)) {
        if (selected_noise_type != noise_type) {
            noise_type = selected_noise_type;
            Update_noise();
        }
    }
    else if (ImGui::RadioButton("High frequency noise(black&white)", &selected_noise_type, 2)) {
        if (selected_noise_type != noise_type) {
            noise_type = selected_noise_type;
            Update_noise();
        }
    }
    else if (ImGui::RadioButton("Spot noise", &selected_noise_type, 3)) {
        if (selected_noise_type != noise_type) {
            noise_type = selected_noise_type;
            Update_noise();
        }
    }
    // Choose convolution type
    ImGui::Text("Choose convolution filter");
    static int selected_convolution_type = convolution_type;
    if (ImGui::RadioButton("Gaussian filter", &selected_convolution_type, 0)) {
        if (selected_convolution_type != convolution_type) {
            convolution_type = selected_convolution_type;
            Update_convolution();
        }
    }
    else if (ImGui::RadioButton("Box filter", &selected_convolution_type, 1)) {
        if (selected_convolution_type != convolution_type) {
            convolution_type = selected_convolution_type;
            Update_convolution();
        }
    }
    else if (ImGui::RadioButton("Tent filter", &selected_convolution_type, 2)) {
        if (selected_convolution_type != convolution_type) {
            convolution_type = selected_convolution_type;
            Update_convolution();
        }
    }
    // Change convolution mask size
    ImGui::Text("Change convolution mask size ");
    ImGui::SameLine();
    if (ImGui::Button("+")) {
        convolution_mask ++;
        Update_convolution();
    }
    ImGui::SameLine();
    if (ImGui::Button("-")) {
        if (convolution_mask > 1)convolution_mask --;
        Update_convolution();
    }
    string s = "Current size: " + to_string(convolution_mask);
    ImGui::Text(s.c_str());

    // Color
    ImGui::Text("Color");
    bool enable_color = show_color;
    ImGui::Checkbox("Show color", &enable_color);
    if (enable_color != show_color) {
        show_color = enable_color;
        Configure_image_vertex_attrib();
    }

    ImGui::End();
}
void Draw_file_panel(GLFWwindow* window) {
    ImGui::Begin("Choose file");
    static int selected_item = 0;
    if (ImGui::BeginCombo("Filename", filename[selected_item], ImGuiComboFlags_None)) {
        for (int i = 0; i < 21; i++) {
            bool is_selected = (selected_item == i);
            if (ImGui::Selectable(filename[i], is_selected)) {
                selected_item = i;
                // reset
                for (int j = 0; j < L; j++) {
                    vector_field[i].clear();
                    flag[i].clear();
                    noise[i].clear();
                }
                vector_field.clear();
                flag.clear();
                noise.clear();
                streamlines.clear();
                // load data
                Read_testing_data_file(i);
                L_image = L * mult_factor, W_image = W * mult_factor;
                L_screen = L_image * 2, W_screen = W_image;
                glViewport(0, 0, L_screen, W_screen);
                if (noise_type == 0)Create_noise_texture_low_frequency();
                else if (noise_type == 1)Create_noise_texture_gray();
                else if (noise_type == 2)Create_noise_texture_white_black();
                else Create_noise_texture_spot();
                RK2();
                state_file = true;
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }
    string s_vsize = "Vector field size: " + to_string(L)+" x "+ to_string(W);
    string s_isize = "Image size: " + to_string(L_image) + " x " + to_string(W_image);
    ImGui::Text(s_vsize.c_str());
    ImGui::Text(s_isize.c_str());
    ImGui::End();
}
void ColorBar(ImDrawList* drawList, ImVec2 pos, float width, float height) {
    const int numSegments = 100;
    const float segmentWidth = width / numSegments;
    for (int i = 0; i < numSegments; ++i) {
        float ratio = static_cast<float>(i) / numSegments;
        glm::vec3 v3 = Calculate_color(ratio);
        ImVec4 color = ImVec4(v3.x, v3.y, v3.z, 1.0);
        ImU32 col = ImGui::ColorConvertFloat4ToU32(color);
        drawList->AddRectFilled(
            ImVec2(pos.x + i * segmentWidth, pos.y),
            ImVec2(pos.x + (i + 1) * segmentWidth, pos.y + height),
            col
        );
    }
}
void Draw_colorbar() {
    ImGui::Begin("Color Bar");

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float width = ImGui::GetContentRegionAvail().x;
    float height = 20.0f;
    ColorBar(drawList, pos, width, height);
    ImGui::Dummy(ImVec2(width, height));

    ImGui::End();
}
int main(int argc, char** argv) {
    // initialize and configure
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // glfw window creation
    //GLFWwindow* window = glfwCreateWindow(1500, 1000, "LIC method", NULL, NULL);
    GLFWwindow* window = glfwCreateWindow(L_screen, W_screen, "LIC method", NULL, NULL);
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
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    Init();
    Create_noise_texture_low_frequency();
    RK2();
    Configure_noise_vertex_attrib();
    Configure_image_vertex_attrib();
    // Render loop
    while (!glfwWindowShouldClose(window)) {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGuiIO& io = ImGui::GetIO();

        Draw_file_panel(window);
        Draw_control_panel();
        Draw_colorbar();
        if (state_file) {
            Configure_noise_vertex_attrib();
            Configure_image_vertex_attrib();
            state_file = !state_file;
        }

        ImGui::ShowDemoWindow();

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glClearColor(background_color[0], background_color[1], background_color[2], 1.0);

        // Activate shader
        shader->use();

        // Render image
        glLineWidth(4.0f);
        glBindVertexArray(VAO_image);
        glDrawArrays(GL_LINES, 0, point_cnt);
        glBindVertexArray(0); // unbind VAO

        // Render noise
        glBindVertexArray(VAO_noise);
        glDrawArrays(GL_POINTS, 0, L_image * W_image);
        glBindVertexArray(0); // unbind VAO

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        // Swap buffers and poll IO events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &VAO_image);
    glDeleteVertexArrays(1, &VAO_noise);
    glDeleteBuffers(1, &VBO_image);
    glDeleteBuffers(1, &VBO_noise);
    glDeleteProgram(shader->ID);
    glfwTerminate();

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}
void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, L_screen, W_screen);
}