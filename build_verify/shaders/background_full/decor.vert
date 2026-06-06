#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec3 worldNormal;
out vec3 localPos;

void main() {
    vec4 position = model * vec4(aPosition, 1.0);
    worldPos = position.xyz;
    worldNormal = normalize(mat3(transpose(inverse(model))) * aNormal);
    localPos = aPosition;
    gl_Position = projection * view * position;
}
