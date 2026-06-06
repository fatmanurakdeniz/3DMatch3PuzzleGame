#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in mat4 instanceModel;

uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec3 worldNormal;

void main() {
    vec4 position = instanceModel * vec4(aPosition, 1.0);
    worldPos = position.xyz;
    worldNormal = normalize(mat3(transpose(inverse(instanceModel))) * aNormal);
    gl_Position = projection * view * position;
}

