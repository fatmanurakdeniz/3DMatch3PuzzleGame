#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in float aPhase;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;
uniform float time;

out float particleFade;

void main() {
    vec3 position = aPosition;
    position.x += sin(time * 0.16 + aPhase) * 0.22;
    position.y += sin(time * 0.11 + aPhase * 1.7) * 0.15;
    position.z += cos(time * 0.13 + aPhase * 0.8) * 0.18;

    float distanceToCamera = length(position - cameraPos);
    particleFade = exp(-distanceToCamera * 0.055);
    gl_Position = projection * view * vec4(position, 1.0);
    gl_PointSize = mix(1.0, 3.0, particleFade);
}

