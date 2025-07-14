#version 330 core
layout(location = 0) in vec3 aPos;      // world coord
layout(location = 1) in vec3 aTexCoord; // texture coord

out vec3 TexCoord;
out vec3 FragPos;

uniform mat4 view;
uniform mat4 projection;


void main(){
    TexCoord = aTexCoord;
    FragPos = aPos;
    gl_Position = projection * view * vec4(aPos, 1.0);
}