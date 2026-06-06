#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform vec4 effectColor;
uniform int effectMode;
uniform float progress;

float softCircle(vec2 uv, float radius, float softness) {
    float d = length(uv - vec2(0.5));
    return 1.0 - smoothstep(radius, radius + softness, d);
}

float softRing(vec2 uv, float radius, float thickness, float softness) {
    float d = length(uv - vec2(0.5));
    float outer = 1.0 - smoothstep(radius, radius + softness, d);
    float inner = smoothstep(radius - thickness - softness, radius - thickness, d);
    return outer * inner;
}

float softBeam(vec2 uv) {
    vec2 p = uv - vec2(0.5);
    float body = 1.0 - smoothstep(0.08, 0.28, abs(p.y));
    float caps = 1.0 - smoothstep(0.42, 0.52, abs(p.x));
    float centerGlow = 1.0 - smoothstep(0.00, 0.36, length(vec2(p.x * 0.55, p.y * 2.6)));
    return max(body * caps, centerGlow * 0.55);
}

void main() {
    float alpha = 0.0;

    if (effectMode == 0) {
        // Soft circular flash / particle
        alpha = softCircle(TexCoord, 0.22, 0.28);
    } else if (effectMode == 1) {
        // Long soft beam / rocket trail
        alpha = softBeam(TexCoord);
    } else if (effectMode == 2) {
        // Shockwave ring
        float radius = mix(0.10, 0.46, progress);
        alpha = softRing(TexCoord, radius, 0.055, 0.055);
    } else {
        alpha = softCircle(TexCoord, 0.25, 0.25);
    }

    alpha *= effectColor.a;

    if (alpha < 0.01)
        discard;

    FragColor = vec4(effectColor.rgb, alpha);
}