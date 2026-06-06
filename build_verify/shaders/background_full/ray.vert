#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 worldPos;
out vec2 texCoord;

void main() {
    vec4 position = model * vec4(aPosition, 1.0);
    worldPos = position.xyz;
    texCoord = aTexCoord;
    gl_Position = projection * view * position;
}

