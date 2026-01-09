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

    // Diffuse lighting
    float diff = max(dot(normal, -lightdir), 0.0);
    vec3 diffuse = diff * material_diffuse;
    if (has_diffuse_map == 1) {
        diffuse *= texture(diffuse_map, fragTexCoord).rgb;
    }

    // Specular lighting
    vec3 reflectDir = reflect(lightdir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material_shininess);
    vec3 specular = spec * material_specular;
    if (has_specular_map == 1) {
        specular *= texture(specular_map, fragTexCoord).rgb;
    }

    // Ambient lighting
    vec3 ambient = material_ambient * 0.3;
    if (has_diffuse_map == 1) {
        ambient *= texture(diffuse_map, fragTexCoord).rgb;
    }

    // Final color
    color = vec4(ambient + diffuse + specular, 1.0);
}
