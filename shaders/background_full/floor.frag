#version 330 core

in vec3 worldPos;
in vec3 worldNormal;
in vec2 texCoord;

uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform float time;
uniform sampler2D sandTexture;

out vec4 fragColor;

vec2 hash22(vec2 p) {
    vec2 q = vec2(
        dot(p, vec2(127.1, 311.7)),
        dot(p, vec2(269.5, 183.3))
    );
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
                borderDistance = min(
                    borderDistance,
                    dot(0.5 * (nearestVector + difference), normalize(separation))
                );
            }
        }
    }
    return borderDistance;
}

void main() {
    vec3 waterColor = vec3(0.0, 0.52, 0.62);

    float distanceToCamera = length(worldPos - cameraPos);
    float fogDensity = 0.06;
    float fogDistance = 1.0 - exp(-distanceToCamera * fogDensity);
    float fogHeight = smoothstep(-5.0, 10.0, worldPos.y);
    float atmosphericScatter = smoothstep(8.0, 52.0, distanceToCamera);
    float fogFactor = 1.0
        - (1.0 - fogDistance)
        * (1.0 - fogHeight * 0.12)
        * (1.0 - atmosphericScatter * 0.86);
    fogFactor = clamp(fogFactor, 0.0, 1.0);

    float upwardFacing = clamp(worldNormal.y, 0.0, 1.0);
    vec3 sandColor = texture(sandTexture, texCoord).rgb;
    vec3 normal = normalize(worldNormal);
    float diffuse = max(dot(normal, -sunDirection), 0.0);
    vec3 ambientLight = vec3(0.43, 0.53, 0.46);
    vec3 sunLight = vec3(0.58, 0.64, 0.46) * diffuse;
    vec3 baseColor = sandColor * (ambientLight + sunLight) * mix(0.92, 1.0, upwardFacing);
    baseColor = mix(baseColor, vec3(0.42, 0.50, 0.35), 0.12);

    vec2 causticUv = worldPos.xz * 2.0;
    causticUv += vec2(
        sin(causticUv.y * 0.72 + time * 0.065),
        sin(causticUv.x * 0.64 - time * 0.055)
    ) * 0.12;
    causticUv += vec2(time * 0.007, time * 0.004);
    float border = worleyBorder(causticUv);
    float causticPattern = 1.0 - smoothstep(0.004, 0.020, border);
    causticPattern = smoothstep(0.10, 0.95, causticPattern);

    float causticFade = exp(-distanceToCamera * 0.045) * (1.0 - fogFactor);
    float causticStrength = 0.28;
    float sunTint = clamp(dot(normalize(worldNormal), -sunDirection), 0.0, 1.0);
    vec2 sunFootprint = vec2(-5.0, -12.0) - sunDirection.xz * 14.0;
    float directSun = exp(-length(worldPos.xz - sunFootprint) * 0.045);
    float broadLightVariation = 0.5
        + 0.5 * sin(worldPos.x * 0.032 + time * 0.010)
        * sin(worldPos.z * 0.026 - time * 0.008);
    float secondaryVariation = 0.5
        + 0.5 * sin((worldPos.x + worldPos.z) * 0.018 + time * 0.006);
    float localLight = mix(0.18, 1.0, broadLightVariation * 0.72 + secondaryVariation * 0.28);
    localLight *= mix(0.52, 1.42, directSun);
    float causticGlow = pow(causticPattern, 0.42) * 0.14;
    vec3 causticColor = vec3(0.88, 1.0, 0.68)
        * causticPattern * causticStrength * causticFade * localLight;
    vec3 glowColor = vec3(0.62, 0.76, 0.45) * causticGlow * causticFade * localLight;
    baseColor += (causticColor + glowColor) * mix(0.85, 1.0, sunTint);

    vec3 color = mix(baseColor, waterColor, fogFactor);

    fragColor = vec4(color, 1.0);
}
