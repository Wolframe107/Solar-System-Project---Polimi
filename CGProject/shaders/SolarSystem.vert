#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec2 inTexCoord;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec2 fragTexCoord;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec3 fragPos;
layout(location = 3) out vec3 fragLightPos;

layout(binding = 0) uniform UniformBufferObject {
    mat4 mvp;
    vec3 lightPos;
} ubo;

void main() {
    gl_Position = ubo.mvp * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
    
    // Transform normal to world space
    fragNormal = mat3(transpose(inverse(ubo.mvp))) * inNormal;
    
    // Transform vertex position to world space
    fragPos = vec3(ubo.mvp * vec4(inPosition, 1.0));
    
    // Pass light position to fragment shader
    fragLightPos = ubo.lightPos;
}