#version 330 core
layout (location = 0) in vec3 position;

out vec3 FragPos;

uniform mat4 gFinalMatrix;

void main()
{
    FragPos = position;
    gl_Position = gFinalMatrix * vec4(position, 1.0f);
}