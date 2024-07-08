#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform UniformBufferObject {
    mat4 mMat;
    mat4 mvpMat;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragPos;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.mvpMat * vec4(inPosition, 1.0);
    fragPos = vec3(ubo.mMat * vec4(inPosition, 1.0));
    fragNormal = mat3(transpose(inverse(ubo.mMat))) * inNormal;
    fragTexCoord = inTexCoord;
}