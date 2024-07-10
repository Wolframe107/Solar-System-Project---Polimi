// SolarSystem.frag
#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 lightPos;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lightPos - fragPos);
    
    // Ambient light
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * vec3(1.0);
    
    // Diffuse light (Lambert shading)
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1.0);
    
    // Combine lighting
    vec3 lighting = ambient + diffuse;
    
    // Sample the texture
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;
    
    // Combine lighting with texture color
    vec3 result = lighting * texColor;
    
    outColor = vec4(result, 1.0);
}