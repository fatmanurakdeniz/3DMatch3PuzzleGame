#version 330 core

in vec3 worldPos;
in vec3 worldNormal;
in vec3 localPos;

uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform int animalType;
uniform float time;

out vec4 fragColor;

float causticLight(vec2 p) {
    vec2 q = p * 1.8;
    float waves = sin(q.x + time * 0.42)
        + sin(q.y * 1.3 - time * 0.31)
        + sin((q.x + q.y) * 0.72 + time * 0.24);
    return smoothstep(1.45, 2.55, waves);
}

void main() {
    vec3 normal = normalize(worldNormal);
    float diffuse = max(dot(normal, -sunDirection), 0.0);

    vec3 baseColor;
    if (animalType == 0) {
        float whiteBand = step(0.18, abs(localPos.x)) * (1.0 - step(0.42, abs(localPos.x)));
        float blackEdge = step(0.14, abs(localPos.x)) * (1.0 - step(0.20, abs(localPos.x)));
        blackEdge += step(0.40, abs(localPos.x)) * (1.0 - step(0.47, abs(localPos.x)));
        baseColor = mix(vec3(0.78, 0.24, 0.055), vec3(0.82, 0.84, 0.78), whiteBand);
        baseColor = mix(baseColor, vec3(0.045, 0.055, 0.050), clamp(blackEdge, 0.0, 1.0));
    } else if (animalType == 1) {
        float underside = smoothstep(-0.4, 0.25, -normal.y);
        baseColor = mix(vec3(0.20, 0.34, 0.42), vec3(0.48, 0.62, 0.66), underside);
    } else if (animalType == 2) {
        float silverBand = 0.5 + 0.5 * smoothstep(-0.25, 0.30, normal.y);
        baseColor = mix(vec3(0.22, 0.36, 0.40), vec3(0.62, 0.73, 0.70), silverBand);
    } else if (animalType == 3) {
        float finShade = smoothstep(0.32, 0.58, abs(localPos.y));
        baseColor = mix(vec3(0.66, 0.52, 0.12), vec3(0.80, 0.68, 0.22), finShade);
    } else {
        float reefStripe = 0.5 + 0.5 * sin(localPos.x * 11.0);
        baseColor = mix(vec3(0.08, 0.28, 0.48), vec3(0.16, 0.52, 0.65), reefStripe * 0.35);
    }

    vec3 viewDir = normalize(cameraPos - worldPos);
    vec3 halfVector = normalize(viewDir - sunDirection);
    float specular = pow(max(dot(normal, halfVector), 0.0), 36.0);
    float caustic = causticLight(worldPos.xz) * exp(-length(worldPos - cameraPos) * 0.055);
    vec3 ambient = vec3(0.68, 0.74, 0.70);
    vec3 directional = vec3(0.48, 0.54, 0.48) * diffuse;
    vec3 color = baseColor * (ambient + directional);
    color += vec3(0.78, 1.0, 0.88) * specular * 0.28;
    color += vec3(1.0, 0.92, 0.65) * caustic * 0.12;
    float distanceToCamera = length(worldPos - cameraPos);
    float fogFactor = clamp(1.0 - exp(-distanceToCamera * 0.06), 0.0, 1.0);
    vec3 attenuation = exp(-vec3(0.018, 0.008, 0.005) * distanceToCamera);
    color *= attenuation;
    color = mix(color, vec3(0.0, 0.52, 0.62), fogFactor * 0.68);
    fragColor = vec4(color, 1.0);
}
