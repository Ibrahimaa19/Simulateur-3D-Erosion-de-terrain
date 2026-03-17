#version 330

layout(location = 0) out vec4 fragColor;
in vec4 color;
in vec2 texCoord;
in vec3 WorldPos;

uniform sampler2D terrainTexture0;
uniform sampler2D terrainTexture1;
uniform sampler2D terrainTexture2;
uniform sampler2D terrainTexture3;

uniform float gHeight0 = 64.0;
uniform float gHeight1 = 128.0;
uniform float gHeight2 = 193.0;
uniform float gHeight3 = 256.0;


vec4 CalcTexColor()
{
    vec4 TexColor;
    float Height = WorldPos.y;

    if (Height < gHeight0) {
        TexColor = texture(terrainTexture0, texCoord);
    } 
    else if (Height < gHeight1) {
        vec4 Color0 = texture(terrainTexture0, texCoord);
        vec4 Color1 = texture(terrainTexture1, texCoord);  
        float Delta = gHeight1 - gHeight0;
        float Factor = (Height - gHeight0) / Delta;
        TexColor = mix(Color0, Color1, Factor);
    } 
    else if (Height < gHeight2) {
        vec4 Color0 = texture(terrainTexture1, texCoord); 
        vec4 Color1 = texture(terrainTexture2, texCoord);  
        float Delta = gHeight2 - gHeight1;
        float Factor = (Height - gHeight1) / Delta;
        TexColor = mix(Color0, Color1, Factor);
    } 
    else if (Height < gHeight3) {
        vec4 Color0 = texture(terrainTexture2, texCoord); 
        vec4 Color1 = texture(terrainTexture3, texCoord);  
        float Delta = gHeight3 - gHeight2;
        float Factor = (Height - gHeight2) / Delta;
        TexColor = mix(Color0, Color1, Factor);
    } 
    else {
        TexColor = texture(terrainTexture3, texCoord); 
    }

    return TexColor;
}

void main()
{
    vec4 TexColor = CalcTexColor();
    fragColor = color * TexColor;  
}