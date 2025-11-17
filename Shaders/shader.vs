#version 330 core

layout(location = 0) in vec3 Position; // vertex position
layout(location = 1) in vec3 aColor;   // vertex color

out vec3 Color; // color passed to fragment shader

uniform mat4 gFinalMatrix; // MVP matrix

void main()
{
    gl_Position = gFinalMatrix * vec4(Position, 1.0);
    Color = aColor;
}
