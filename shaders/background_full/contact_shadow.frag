#version 330 core

in vec2 shadowCoord;
in vec3 worldPos;

uniform vec3 cameraPos;

out vec4 fragColor;

void main() {
    float radial = dot(shadowCoord, shadowCoord);
    float alpha = (1.0 - smoothstep(0.05, 1.0, radial)) * 0.28;
    float fog = clamp(1.0 - exp(-length(worldPos - cameraPos) * 0.06), 0.0, 1.0);
    fragColor = vec4(vec3(0.035, 0.085, 0.075), alpha * (1.0 - fog));
}
