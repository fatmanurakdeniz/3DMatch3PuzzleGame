#version 330 core

layout (location = 0) in vec3 aPosition;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float swayOffset;

out vec3 worldPos;
out vec2 texCoord;

void main() {
    vec3 position = aPosition;
    float hangingAmount = 1.0 - aTexCoord.y;
    position.x += sin(time * 0.42 + swayOffset + hangingAmount * 3.2) * 0.055 * hangingAmount;
    vec4 transformed = model * vec4(position, 1.0);
    worldPos = transformed.xyz;
    texCoord = aTexCoord;
    gl_Position = projection * view * transformed;
}
