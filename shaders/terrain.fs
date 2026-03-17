#version 330

layout(location = 0) out vec4 fragColor;
in vec4 color;
in vec2 texCoord;

uniform sampler2D terrainTexture;

void main()
{

    vec3 aColor = texture(terrainTexture, texCoord).rgb;
    fragColor = vec4(aColor,1.0);
}