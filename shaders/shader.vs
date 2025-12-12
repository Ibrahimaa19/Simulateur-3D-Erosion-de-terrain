#version 330 core

layout(location = 0) in vec3 aPos; // vertex position

out float height;

uniform mat4 gFinalMatrix; // MVP matrix

void main()
{
    vec4 v = vec4(aPos.x, aPos.y/10., aPos.z, 1.0); 
    height = aPos.y/10.;

    gl_Position = gFinalMatrix * v;
}
