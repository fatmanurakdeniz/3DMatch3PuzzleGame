#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in mat4 instanceModel;
layout (location = 6) in float instancePhase;

uniform mat4 view;
uniform mat4 projection;
uniform float time;

out vec3 worldPos;
out vec3 worldNormal;

void main() {
    vec3 animatedPosition = aPosition;
    float tailMask = smoothstep(0.35, 1.0, -aPosition.x);
    animatedPosition.z += sin(time * 5.5 + instancePhase + aPosition.x * 5.0) * tailMask * 0.20;
    vec4 position = instanceModel * vec4(animatedPosition, 1.0);
    worldPos = position.xyz;
    worldNormal = normalize(mat3(transpose(inverse(instanceModel))) * aNormal);
    gl_Position = projection * view * position;
}
