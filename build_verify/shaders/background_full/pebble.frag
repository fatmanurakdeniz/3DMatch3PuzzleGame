#version 330 core

in vec3 worldPos;
in vec3 worldNormal;

uniform vec3 cameraPos;
uniform vec3 sunDirection;
uniform float time;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(worldNormal);
    float diffuse = max(dot(normal, -sunDirection), 0.0);
    float variation = 0.5 + 0.5 * sin(worldPos.x * 3.7 + worldPos.z * 4.1);
    vec3 baseColor = mix(vec3(0.20, 0.19, 0.16), vec3(0.42, 0.39, 0.30), variation);
    vec3 color = baseColor * (vec3(0.52, 0.60, 0.56) + vec3(0.50, 0.54, 0.44) * diffuse);
    float caustic = smoothstep(0.92, 0.99, abs(sin(worldPos.x * 3.6 + time * 0.03) * sin(worldPos.z * 3.2 - time * 0.025)));
    color += vec3(1.0, 0.92, 0.65) * caustic * max(normal.y, 0.0) * 0.16;
    float distanceToCamera = length(worldPos - cameraPos);
    float fogFactor = clamp(1.0 - exp(-distanceToCamera * 0.06), 0.0, 1.0);
    color = mix(color, vec3(0.0, 0.52, 0.62), fogFactor);
    fragColor = vec4(color, 1.0);
}

