#version 330

layout(location = 0) out vec4 fragColor;

in vec2 texCoord;
in vec3 WorldPos;
in float HeightRatio;

uniform sampler2D terrainTexture0;
uniform sampler2D terrainTexture1;
uniform sampler2D terrainTexture2;
uniform sampler2D terrainTexture3;

uniform vec3 gCameraPos;
uniform vec3 gLightDirection;
uniform vec3 gFogColor;
uniform float gFogStart;
uniform float gFogEnd;

vec3 terrainNormal()
{
    vec3 dx = dFdx(WorldPos);
    vec3 dz = dFdy(WorldPos);
    vec3 normal = normalize(cross(dz, dx));

    if (normal.y < 0.0)
    {
        normal = -normal;
    }

    return normal;
}

vec3 sampleTerrainTexture()
{
    vec4 valley = texture(terrainTexture3, texCoord * 0.65);
    vec4 soil = texture(terrainTexture0, texCoord * 0.85);
    vec4 grass = texture(terrainTexture2, texCoord * 1.15);
    vec4 rock = texture(terrainTexture1, texCoord * 1.45);

    vec3 lowBlend = mix(valley.rgb, soil.rgb, smoothstep(0.04, 0.22, HeightRatio));
    vec3 midBlend = mix(lowBlend, grass.rgb, smoothstep(0.14, 0.52, HeightRatio));
    vec3 highBlend = mix(midBlend, rock.rgb, smoothstep(0.48, 0.92, HeightRatio));

    return highBlend;
}

vec3 altitudeTint(float slope)
{
    vec3 wetValley = vec3(0.16, 0.22, 0.18);
    vec3 meadow = vec3(0.42, 0.50, 0.27);
    vec3 dryRock = vec3(0.58, 0.53, 0.45);
    vec3 snow = vec3(0.86, 0.90, 0.88);

    vec3 tint = mix(wetValley, meadow, smoothstep(0.05, 0.38, HeightRatio));
    tint = mix(tint, dryRock, smoothstep(0.42, 0.82, HeightRatio));
    tint = mix(tint, snow, smoothstep(0.88, 0.98, HeightRatio));

    float rockExposure = smoothstep(0.35, 0.85, slope);
    tint = mix(tint, dryRock, rockExposure * 0.45);

    return tint;
}

void main()
{
    vec3 normal = terrainNormal();
    vec3 lightDir = normalize(-gLightDirection);
    vec3 viewDir = normalize(gCameraPos - WorldPos);

    float diffuse = max(dot(normal, lightDir), 0.0);
    float halfLambert = diffuse * 0.72 + 0.28;

    float slope = 1.0 - clamp(normal.y, 0.0, 1.0);
    vec3 baseColor = sampleTerrainTexture();
    vec3 tint = altitudeTint(slope);

    vec3 color = mix(baseColor, tint, 0.46);

    float rim = pow(1.0 - max(dot(normal, viewDir), 0.0), 2.0);
    vec3 ambient = vec3(0.18, 0.22, 0.26);
    vec3 sunColor = vec3(1.0, 0.92, 0.78);
    vec3 skyBounce = vec3(0.12, 0.16, 0.20) * normal.y;

    color *= ambient + skyBounce + sunColor * halfLambert;
    color += vec3(0.18, 0.23, 0.27) * rim * 0.18;

    float distanceToCamera = length(gCameraPos - WorldPos);
    float fog = smoothstep(gFogStart, gFogEnd, distanceToCamera);
    color = mix(color, gFogColor, fog);

    fragColor = vec4(pow(color, vec3(1.0 / 2.2)), 1.0);
}
