#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform vec3 cameraPos;

out vec3 worldPos;
out vec3 worldNormal;
out vec2 texCoord;

void main() {
    float nearCamera = exp(-length(aPosition.xz - cameraPos.xz) * 0.055);
    float amplitude = mix(2.0, 3.3, nearCamera);
    float wave1 = sin(aPosition.x * 0.8 + time * 0.7) * 0.15;
    float wave2 = sin(aPosition.z * 1.2 + time * 0.5) * 0.10;
    float wave3 = sin((aPosition.x + aPosition.z) * 0.4 + time * 0.3) * 0.08;
    worldPos = aPosition + vec3(0.0, (wave1 + wave2 + wave3) * amplitude, 0.0);

    float dHeightDx = (
        cos(aPosition.x * 0.8 + time * 0.7) * 0.120
        + cos((aPosition.x + aPosition.z) * 0.4 + time * 0.3) * 0.032
    ) * amplitude;
    float dHeightDz = (
        cos(aPosition.z * 1.2 + time * 0.5) * 0.120
        + cos((aPosition.x + aPosition.z) * 0.4 + time * 0.3) * 0.032
    ) * amplitude;
    worldNormal = normalize(vec3(-dHeightDx, 1.0, -dHeightDz));
    texCoord = aTexCoord;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
