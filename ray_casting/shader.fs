
#version 330 core
in vec3 TexCoord;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler3D texture3D;
uniform sampler1D texture1D;
uniform vec3 d; // ray direction
uniform float t; // gap
uniform vec3 eye;
uniform int projection_mode; // 0:ortho 1:persp
uniform vec3 lightPos;
uniform bool Phong;
vec3 phong(vec3 position, vec3 gradient, vec3 color){
    gradient= 2.0 * gradient - 1.0;
    vec3 lightcolor = vec3(1,1,1);

    // ambient
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightcolor; 

    // diffuse 
    vec3 norm = normalize(gradient);
    vec3 lightDir = normalize(lightPos -  position);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightcolor;

    // specular
    float specularStrength = 0.5;
    vec3 viewDir = normalize(eye - position); 
    vec3 reflectDir = reflect(-lightDir, norm);  
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightcolor; 

    vec3 result = (ambient + diffuse + specular) * color;
    return result;
}
bool outside(vec3 texCoord){
    if(
        texCoord.x > 1 || texCoord.x < 0 || 
        texCoord.y > 1 || texCoord.y < 0 ||
        texCoord.z > 1 || texCoord.z < 0 
    )return true;
    return false;
}
void main(){
    float T; // opacity
    float threshold = 0.9;
    vec3 color, tempColor;
    vec4 texel1, texel2;
    vec3 P = FragPos, P0 = FragPos;
    vec3 texCoord = TexCoord, texCoord0 = TexCoord;
    vec3 dir=d;

    T = 0.0;
    color = vec3(0.0, 0.0, 0.0);
    //if(projection_mode == 1)dir = normalize(FragPos-eye);
    while(true){
        texel1 = texture(texture3D, texCoord); // 3d texture: (gradient, 1d texture coord)
        texel2 = texture(texture1D, texel1.a); // 1d texture: (color, opacity)
        if(Phong){
            tempColor = phong(P, texel1.rgb, texel2.rgb); // position, gradient, color
            color = color + (1.0 - T) * tempColor * texel2.a;
        }
        else color = color + (1.0 - T) * texel2.rgb * texel2.a;
        T = T + (1.0 - T) * texel2.a;
        P = P + t * dir;
        if(projection_mode == 1)dir = normalize(P-eye);
        texCoord = texCoord0 + (P - P0) / 255.0;
        if(outside(texCoord))break;
        if(T>threshold)break;
    }
    FragColor = vec4(color, min(T, threshold));
}

