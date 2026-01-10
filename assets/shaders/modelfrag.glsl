#version 330 core

in vec3 fragPos;
in vec3 fragNormal;
in vec2 fragTexCoord;

out vec4 color;

uniform vec3 lightdir;
uniform vec3 camerapos;

// Material properties
uniform vec3 material_ambient;
uniform vec3 material_diffuse;
uniform vec3 material_specular;
uniform float material_shininess;

// Textures
uniform sampler2D diffuse_map;
uniform sampler2D specular_map;
uniform int has_diffuse_map;
uniform int has_specular_map;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 viewDir = normalize(camerapos - fragPos);

    // Base color from material (ensure minimum brightness)
    vec3 baseColor = max(material_diffuse, vec3(0.5));
    
    // Apply texture if available
    if (has_diffuse_map == 1) {
        vec4 texColor = texture(diffuse_map, fragTexCoord);
        if (texColor.a > 0.01) {
            baseColor = texColor.rgb;
        }
    }

    // Simple lighting: high ambient + diffuse
    float diff = abs(dot(normal, -lightdir));
    float lighting = 0.5 + 0.5 * diff; // 50% ambient, 50% diffuse
    
    // Ensure minimum brightness
    vec3 finalColor = max(baseColor * lighting, vec3(0.1));
    
    // Final color
    color = vec4(finalColor, 1.0);
}
