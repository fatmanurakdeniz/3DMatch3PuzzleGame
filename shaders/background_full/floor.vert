#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;

float seabedHeight(vec2 p) {
    float dunes = sin(p.x * 0.085 + sin(p.y * 0.045)) * 0.15
        + sin(p.y * 0.11 - p.x * 0.025) * 0.08;
    float ripples = sin(p.x * 1.45 + p.y * 0.18) * 0.018
        + sin(p.y * 1.15 - p.x * 0.12) * 0.012;
    float depressions = -exp(-dot(p - vec2(-1.5, -4.0), p - vec2(-1.5, -4.0)) * 0.045) * 0.16
        - exp(-dot(p - vec2(4.0, -10.5), p - vec2(4.0, -10.5)) * 0.060) * 0.12;
    float rockMounds = exp(-dot(p - vec2(-5.2, -2.0), p - vec2(-5.2, -2.0)) * 0.10) * 0.20
        + exp(-dot(p - vec2(5.8, -11.5), p - vec2(5.8, -11.5)) * 0.085) * 0.18;
    return dunes + ripples + depressions + rockMounds;
}

void main() {
    vec3 displaced = aPosition;
    displaced.y += seabedHeight(aPosition.xz);
    float epsilon = 0.18;
    float heightX = seabedHeight(aPosition.xz + vec2(epsilon, 0.0));
    float heightZ = seabedHeight(aPosition.xz + vec2(0.0, epsilon));
    vec3 displacedNormal = normalize(vec3(
        -(heightX - displaced.y) / epsilon,
        1.0,
        -(heightZ - displaced.y) / epsilon
    ));
    vec4 worldPosition = model * vec4(displaced, 1.0);
    worldPos = worldPosition.xyz;
    worldNormal = normalize(mat3(transpose(inverse(model))) * displacedNormal);
    texCoord = aTexCoord;
    gl_Position = projection * view * worldPosition;
}
