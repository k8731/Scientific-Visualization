#version 330 core
layout(location = 0) in vec2 aPos;      
layout(location = 1) in vec3 aColor;

out vec3 vertexColor;

void main(){
    gl_Position = vec4(aPos.xy, 0.0, 1.0);
    vertexColor = aColor;
}