#version 330

layout(location = 0) in vec3 position;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 gFinalMatrix;
uniform float gMinHeight;
uniform float gMaxHeight;

out vec2 texCoord;
out vec3 WorldPos;
out float HeightRatio;

void main()
{
    gl_Position = gFinalMatrix * vec4(position, 1.0);

    float deltaHeight = max(gMaxHeight - gMinHeight, 0.0001);
    HeightRatio = clamp((position.y - gMinHeight) / deltaHeight, 0.0, 1.0);

    texCoord = aTexCoord;
    WorldPos = position;
}
