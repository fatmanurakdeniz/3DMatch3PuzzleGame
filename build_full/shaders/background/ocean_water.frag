#version 330 core

in vec2 screenUv;

uniform float time;
uniform vec3 sunDirection;
uniform vec4 screenInfo;

out vec4 fragColor;

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 cell = floor(p);
    vec2 local = fract(p);
    local = local * local * (3.0 - 2.0 * local);

    float a = hash(cell);
    float b = hash(cell + vec2(1.0, 0.0));
    float c = hash(cell + vec2(0.0, 1.0));
    float d = hash(cell + vec2(1.0, 1.0));
    return mix(mix(a, b, local.x), mix(c, d, local.x), local.y);
}

void main() {
    vec2 uv = screenUv;
    float aspect = max(screenInfo.x / max(screenInfo.y, 1.0), 1.0);

    vec3 topColor = vec3(0.02, 0.82, 0.90);
    vec3 midColor = vec3(0.0, 0.48, 0.58);
    vec3 deepColor = vec3(0.0, 0.19, 0.27);

    float lowerBlend = smoothstep(0.0, 0.48, uv.y);
    float upperBlend = smoothstep(0.42, 1.0, uv.y);
    vec3 waterColor = mix(deepColor, midColor, lowerBlend);
    waterColor = mix(waterColor, topColor, upperBlend);

    float horizonHaze = exp(-pow((uv.y - 0.43) / 0.25, 2.0));
    waterColor = mix(waterColor, midColor, horizonHaze * 0.66);

    vec2 surfaceUv = uv * vec2(7.0 * aspect, 4.0);
    float surfaceNoise1 = noise(surfaceUv + vec2(time * 0.035, time * 0.018));
    float surfaceNoise2 = noise(surfaceUv * 1.8 + vec2(-time * 0.025, time * 0.03));
    float surfacePattern = smoothstep(0.58, 0.88, surfaceNoise1 * 0.62 + surfaceNoise2 * 0.38);
    float surfaceMask = smoothstep(0.48, 0.98, uv.y);
    waterColor += vec3(0.03, 0.11, 0.12) * surfacePattern * surfaceMask;

    vec2 sunScreenPosition = vec2(0.40 + sunDirection.x * 0.04, 1.02);
    vec2 sunDelta = (uv - sunScreenPosition) / vec2(0.58, 0.34);
    float sunGlow = exp(-dot(sunDelta, sunDelta) * 3.4);
    waterColor += vec3(0.36, 0.43, 0.34) * sunGlow;

    float scattering = noise(uv * vec2(5.0 * aspect, 8.0) + time * 0.008);
    waterColor += vec3(0.0, 0.018, 0.022) * scattering;

    fragColor = vec4(waterColor, 1.0);
}
