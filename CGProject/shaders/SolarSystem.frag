#version 450

layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragPos;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    vec3 lightPos;
} ubo;

layout(binding = 1) uniform sampler2D texSampler;

void main() {
    vec3 norm = normalize(fragNormal);
    vec3 lightDir = normalize(ubo.lightPos - fragPos);

    // Basic diffuse lighting
    float diff = max(dot(norm, lightDir), 0.0);

    // Quantize the diffuse to create cel shading effect
    float levels = 4.0;
    diff = floor(diff * levels) / levels;

    // Sample the texture
    vec3 texColor = texture(texSampler, fragTexCoord).rgb;

    // Apply cel shading
    vec3 celColor = texColor * diff;

    // Add an outline
    float outline = 1.0;
    if (dot(norm, vec3(0.0, 0.0, 1.0)) < 0.2) {
        outline = 0.0;
    }

    // Combine cel shading and outline
    vec3 finalColor = mix(vec3(0.0), celColor, outline);

    // Add a subtle ambient light to prevent completely black areas
    finalColor += texColor * 0.1;

    // Output final color without transparency
    outColor = vec4(finalColor, 1.0);
}