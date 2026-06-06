#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform int animalType;
uniform float animationPhase;
uniform float swimSpeed;

out vec3 worldPos;
out vec3 worldNormal;
out vec3 localPos;

void main() {
    vec3 animatedPosition = aPosition;
    float tailMask = smoothstep(0.15, 0.85, -aPosition.x);
    float tailSpeed = animalType == 0 ? 5.5 : (animalType == 1 ? 2.2 : swimSpeed);
    float tailAmplitude = animalType == 0 ? 0.10 : (animalType == 1 ? 0.06 : 0.22);
    float wave = sin(time * tailSpeed + animationPhase + aPosition.x * 4.5);
    animatedPosition.z += wave * tailMask * tailAmplitude;
    if (animalType >= 2) {
        float bodyMask = 1.0 - smoothstep(-0.72, -0.15, aPosition.x);
        animatedPosition.z += wave * bodyMask * 0.045;
    }
    vec4 position = model * vec4(animatedPosition, 1.0);
    worldPos = position.xyz;
    worldNormal = normalize(mat3(transpose(inverse(model))) * aNormal);
    localPos = aPosition;
    gl_Position = projection * view * position;
}
