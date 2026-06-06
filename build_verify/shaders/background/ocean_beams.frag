#version 330 core

in vec2 screenUv;

uniform float time;
uniform vec4 screenInfo;

out vec4 fragColor;

float softBeam(vec2 uv, float center, float slope, float width) {
    float beamCenter = center + slope * (1.0 - uv.y);
    float distanceFromBeam = abs(uv.x - beamCenter);
    return exp(-distanceFromBeam * distanceFromBeam / width);
}

void main() {
    vec2 uv = screenUv;
    float aspect = max(screenInfo.x / max(screenInfo.y, 1.0), 1.0);
    uv.x = (uv.x - 0.5) * aspect + 0.5;

    float shimmer = 0.86 + 0.14 * sin(time * 0.45);
    float vertical = smoothstep(0.10, 0.94, uv.y) * (1.0 - smoothstep(0.70, 1.0, uv.y));

    float rays = 0.0;
    rays += softBeam(uv, 0.28, -0.20, 0.010);
    rays += softBeam(uv, 0.42, -0.12, 0.015);
    rays += softBeam(uv, 0.58, -0.06, 0.012);
    rays += softBeam(uv, 0.72,  0.04, 0.018);

    float alpha = clamp(rays * vertical * shimmer * 0.12, 0.0, 0.22);
    fragColor = vec4(0.78, 0.98, 1.0, alpha);
}
