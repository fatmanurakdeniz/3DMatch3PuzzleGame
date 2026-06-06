#version 330 core

layout (location = 0) in vec2 basePos;
layout (location = 1) in float phase;
layout (location = 2) in float size;

uniform float time;
uniform vec4 screenInfo;

out float particleFade;

void main() {
    vec2 p = basePos;
    p.y = mod(p.y + time * 0.035 + 1.2, 2.4) - 1.2;
    p.x += sin(time * 0.32 + phase) * 0.025;

    float aspectCompensation = max(screenInfo.x / max(screenInfo.y, 1.0), 1.0);
    vec2 ndc = vec2(p.x / aspectCompensation, p.y);

    float depthFade = smoothstep(-1.1, -0.35, p.y) * (1.0 - smoothstep(0.55, 1.1, p.y));
    particleFade = depthFade * (0.72 + 0.28 * sin(time * 0.8 + phase));
    gl_PointSize = size * (0.78 + 0.22 * sin(time * 1.3 + phase));
    gl_Position = vec4(ndc, 0.0, 1.0);
}
