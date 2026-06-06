#version 330 core

in float particleFade;

out vec4 fragColor;

void main() {
    vec2 point = gl_PointCoord - vec2(0.5);
    float softCircle = 1.0 - smoothstep(0.08, 0.50, length(point));
    float alpha = softCircle * particleFade * 0.24;
    fragColor = vec4(vec3(0.68, 0.96, 0.94), alpha);
}
