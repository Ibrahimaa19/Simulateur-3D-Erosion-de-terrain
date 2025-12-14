#version 330

layout (location = 0) in vec3 position;

uniform mat4 gFinalMatrix;
uniform float gMinHeight;
uniform float gMaxHeight;
out vec4 color;
void main()
{
    gl_Position = gFinalMatrix * vec4(position, 1.0f);
    float deltaHeight = gMaxHeight - gMinHeight;
    float heightRatio = (position.y - gMinHeight) / deltaHeight;
    float c = heightRatio * 0.8 + 0.2;
    color = vec4(c, c, c, 1.0f);
}