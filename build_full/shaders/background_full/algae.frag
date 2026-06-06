#version 330 core

in vec3 worldPos;
in vec2 texCoord;

uniform vec3 cameraPos;
uniform float time;
uniform float swayOffset;

out vec4 fragColor;

void main() {
    float wave = 0.09 * sin(texCoord.y * 7.0 + time * 0.35 + swayOffset);
    float taper = mix(0.020, 0.055, texCoord.y);
    float strip = 1.0 - smoothstep(taper, taper + 0.025, abs(texCoord.x - (0.5 + wave)));
    float hangingFade = smoothstep(0.0, 0.12, texCoord.y)
        * (1.0 - smoothstep(0.82, 1.0, texCoord.y));
    float fogFade = exp(-length(worldPos - cameraPos) * 0.055);
    float alpha = strip * hangingFade * fogFade * 0.34;
    if (alpha < 0.01) {
        discard;
    }
    fragColor = vec4(vec3(0.025, 0.20, 0.14), alpha);
}
