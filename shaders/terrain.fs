#version 330 core
out vec4 FragColor;

in vec3 FragPos;

uniform float gMaxHeight;
uniform float gMinHeight;
uniform float uHasWater;      // 1.0 si eau présente, 0.0 sinon
uniform float uWaterLevel;    // Niveau où l'eau commence

void main()
{
    float height = FragPos.y;
    float t = (height - gMinHeight) / (gMaxHeight - gMinHeight + 0.001);
    
    // Déterminer si on est sous l'eau
    bool underWater = (uHasWater > 0.5) && (height < uWaterLevel);
    
    vec3 color;
    
    if (underWater) {
        // COULEUR EAU - BLEU visible
        float depth = (uWaterLevel - height) / 5.0; // 5 unités de profondeur max
        depth = clamp(depth, 0.0, 1.0);
        
        // Bleu plus foncé en profondeur
        color = mix(vec3(0.1, 0.3, 0.9),   // Bleu clair (surface)
                    vec3(0.0, 0.1, 0.6),   // Bleu foncé (profondeur)
                    depth);
        
        // Petites vagues à la surface
        if (depth < 0.2) {
            float wave = sin(FragPos.x * 3.0) * cos(FragPos.z * 3.0) * 0.05;
            color += vec3(wave, wave * 0.5, 0.0);
        }
    } else {
        // COULEUR TERRAIN - par altitude
        if (t < 0.2) {
            color = vec3(0.9, 0.85, 0.6);  // Sable
        } else if (t < 0.5) {
            color = vec3(0.2, 0.7, 0.3);   // Herbe verte
        } else if (t < 0.8) {
            color = vec3(0.5, 0.5, 0.5);   // Roche
        } else {
            color = vec3(0.95, 0.95, 0.95);// Neige
        }
        
        // Légère variation pour plus de réalisme
        float noise = fract(sin(dot(FragPos.xz, vec2(12.9898, 78.233))) * 43758.5453);
        color += (noise - 0.5) * 0.1;
    }
    
    // Éclairage simple
    float light = 0.6 + 0.4 * t;
    color *= light;
    
    // Assurer des couleurs valides
    color = clamp(color, 0.0, 1.0);
    
    FragColor = vec4(color, 1.0);
}