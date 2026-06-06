#version 330 core

in vec3 worldPos;
in vec3 worldNormal;
in vec3 localPos;

uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform vec3 baseColor;
uniform float time;
uniform int shellDetail;

out vec4 fragColor;

float causticField(vec2 p) {
    p += vec2(sin(p.y * 0.7 + time * 0.06), sin(p.x * 0.6 - time * 0.05)) * 0.10;
    float lines = abs(sin(p.x * 2.1 + sin(p.y * 1.7)) * sin(p.y * 2.0 - sin(p.x * 1.5)));
    return smoothstep(0.90, 0.99, lines);
}

void main() {
    vec3 normal = normalize(worldNormal);
    float diffuse = max(dot(normal, -sunDirection), 0.0);
    vec3 materialColor = baseColor;
    if (shellDetail == 1) {
        float colorVariation = 0.94 + 0.06 * sin(localPos.x * 17.0 + localPos.z * 11.0);
        materialColor *= colorVariation;
    }
    vec3 color = materialColor * (vec3(0.52, 0.62, 0.58) + vec3(0.55, 0.60, 0.48) * diffuse);

    if (shellDetail == 1) {
        float cavityOcclusion = mix(0.68, 1.0, smoothstep(-0.15, 0.65, normal.y));
        float wornEdge = smoothstep(0.42, 0.93, abs(sin(localPos.x * 13.0 + localPos.z * 9.0)))
            * smoothstep(0.30, 0.85, length(localPos.xz));
        color *= cavityOcclusion;
        color += vec3(0.20, 0.16, 0.11) * wornEdge * 0.16;
    } else if (shellDetail == 2) {
        float branchOcclusion = mix(0.64, 1.0, smoothstep(-0.35, 0.75, normal.y));
        float organicVariation = 0.90 + 0.10 * sin(localPos.y * 9.0 + localPos.x * 5.0);
        color *= branchOcclusion * organicVariation;
    }

    float distanceToCamera = length(worldPos - cameraPos);
    float lightPatch = 0.5 + 0.5 * sin(worldPos.x * 1.8 + worldPos.z * 1.5 + time * 0.08);
    color += vec3(0.34, 0.45, 0.26) * lightPatch * max(normal.y, 0.0) * 0.10;
    float caustic = causticField(worldPos.xz * 1.8);
    color += vec3(1.0, 0.92, 0.65) * caustic * max(normal.y, 0.0) * 0.16;
    float fogFactor = clamp(1.0 - exp(-distanceToCamera * 0.06), 0.0, 1.0);
    color = mix(color, vec3(0.0, 0.52, 0.62), fogFactor);
    fragColor = vec4(color, 1.0);
}
