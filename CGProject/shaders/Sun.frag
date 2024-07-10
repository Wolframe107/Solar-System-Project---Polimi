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
    // Extend the UV coordinates to create a larger sun
    vec2 extendedUV = (fragTexCoord - 0.5) * 1.5 + 0.5;
    
    // Sample the base texture with extended UVs
    vec4 texColor = texture(texSampler, extendedUV);
    
    // Calculate distance from center for glow and aura effects
    vec2 center = vec2(0.5, 0.5);
    float dist = length(extendedUV - center);
    
    // Create an intense orange glow effect
    float glow = 1.0 - smoothstep(0.0, 0.75, dist);
    vec3 glowColor = vec3(1.0, 0.3, 0.0); // Intense orange
    
    // Combine base texture with glow
    vec3 finalColor = mix(texColor.rgb, glowColor, glow * 0.8);
    
    // Add edge highlight with a bright orange color
    float edge = 1.0 - smoothstep(0.6, 0.75, dist);
    vec3 edgeColor = vec3(1.0, 0.5, 0.1); // Bright orange edge
    finalColor += edgeColor * edge * 0.6;
    
    // Add extended aura effect
    float auraIntensity = smoothstep(0.75, 1.5, dist);
    vec3 auraColor = vec3(1.0, 0.6, 0.2); // Slightly lighter orange for the aura
    finalColor = mix(finalColor, auraColor, auraIntensity * 0.7);
    
    // Increase overall brightness for a more vibrant appearance
    finalColor *= 1.6;
    
    // Add some color variation to simulate solar surface
    float noise = fract(sin(dot(extendedUV, vec2(12.9898, 78.233))) * 43758.5453);
    finalColor += vec3(0.15, 0.05, 0.0) * noise * (1.0 - auraIntensity);
    
    // Ensure the color doesn't exceed maximum brightness
    finalColor = min(finalColor, vec3(1.0));
    
    // Apply a strong orange tint
    vec3 tint = vec3(1.0, 0.85, 0.7); // Strong orange tint
    finalColor *= tint;
    
    // Calculate alpha for the aura (1.0 in the center, fading out towards the edges)
    float alpha = 1.0 - smoothstep(0.75, 1.5, dist);
    alpha = smoothstep(0.0, 0.2, alpha); // Sharpen the alpha transition a bit
    
    outColor = vec4(finalColor, alpha);
}