#version 330 core

layout (location = 0) in vec2 aPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 shadowCoord;
out vec3 worldPos;

void main() {
    shadowCoord = aPosition;
    vec4 position = model * vec4(aPosition.x, 0.0, aPosition.y, 1.0);
    worldPos = position.xyz;
    gl_Position = projection * view * position;
}
