#version 330 core

in vec3 worldPos;
in vec3 worldNormal;

uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform float time;
uniform float rockVariation;

out vec4 fragColor;

vec2 hash22(vec2 p) {
    vec2 q = vec2(dot(p, vec2(127.1, 311.7)), dot(p, vec2(269.5, 183.3)));
    return fract(sin(q) * 43758.5453);
}

float worleyBorder(vec2 p) {
    vec2 cell = floor(p);
    vec2 local = fract(p);
    vec2 nearestCell = vec2(0.0);
    vec2 nearestVector = vec2(0.0);
    float nearestDistance = 10.0;
    for (int y = -1; y <= 1; ++y) {
        for (int x = -1; x <= 1; ++x) {
            vec2 offset = vec2(float(x), float(y));
            vec2 point = 0.5 + 0.34 * sin(time * 0.055 + 6.2831 * hash22(cell + offset));
            vec2 difference = offset + point - local;
            float distanceSquared = dot(difference, difference);
            if (distanceSquared < nearestDistance) {
                nearestDistance = distanceSquared;
                nearestVector = difference;
                nearestCell = offset;
            }
        }
    }
    float borderDistance = 10.0;
    for (int y = -2; y <= 2; ++y) {
        for (int x = -2; x <= 2; ++x) {
            vec2 offset = nearestCell + vec2(float(x), float(y));
            vec2 point = 0.5 + 0.34 * sin(time * 0.055 + 6.2831 * hash22(cell + offset));
            vec2 difference = offset + point - local;
            vec2 separation = difference - nearestVector;
            if (dot(separation, separation) > 0.0001) {
                borderDistance = min(borderDistance, dot(0.5 * (nearestVector + difference), normalize(separation)));
            }
        }
    }
    return borderDistance;
}

void main() {
    vec3 normal = normalize(worldNormal);
    float diffuse = max(dot(normal, -sunDirection), 0.0);
    float variation = 0.5 + 0.5 * sin(rockVariation * 3.1);
    vec3 rockColor = mix(vec3(0.20, 0.19, 0.16), vec3(0.36, 0.31, 0.24), variation);
    vec3 color = rockColor * (vec3(0.58, 0.64, 0.58) + vec3(0.46, 0.50, 0.42) * diffuse);

    vec2 cuv = worldPos.xz * 2.0;
    cuv += vec2(sin(cuv.y * 0.72 + time * 0.065), sin(cuv.x * 0.64 - time * 0.055)) * 0.12;
    cuv += vec2(time * 0.007, time * 0.004);
    float caustic = 1.0 - smoothstep(0.004, 0.020, worleyBorder(cuv));
    float distanceToCamera = length(worldPos - cameraPos);
    float fogFactor = clamp(1.0 - exp(-distanceToCamera * 0.06), 0.0, 1.0);
    color += vec3(1.0, 0.92, 0.65) * caustic * 0.18 * exp(-distanceToCamera * 0.045) * max(normal.y, 0.0);
    color = mix(color, vec3(0.0, 0.52, 0.62), fogFactor);
    fragColor = vec4(color, 1.0);
}
