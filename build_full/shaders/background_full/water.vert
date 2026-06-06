#version 330 core

out vec2 screenUv;

void main() {
    vec2 position = vec2(
        float((gl_VertexID << 1) & 2),
        float(gl_VertexID & 2)
    );
    screenUv = position;
    gl_Position = vec4(position * 2.0 - 1.0, 0.999, 1.0);
}

