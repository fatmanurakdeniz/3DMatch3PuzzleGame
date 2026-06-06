#version 330 core

in vec3 worldPos;
in vec3 worldNormal;

uniform vec3 cameraPos;
uniform vec3 sunDirection;

out vec4 fragColor;

void main() {
    vec3 normal = normalize(worldNormal);
    vec3 viewDir = normalize(cameraPos - worldPos);
    vec3 halfVector = normalize(viewDir - sunDirection);
    float diffuse = max(dot(normal, -sunDirection), 0.0);
    float specular = pow(max(dot(normal, halfVector), 0.0), 42.0);

    vec3 silver = mix(vec3(0.18, 0.42, 0.52), vec3(0.64, 0.78, 0.80), diffuse);
    vec3 color = silver * (0.62 + diffuse * 0.38);
    color += vec3(0.58, 0.86, 0.92) * specular * 0.32;

    float distanceToCamera = length(worldPos - cameraPos);
    vec3 attenuation = exp(-vec3(0.024, 0.012, 0.007) * distanceToCamera);
    color *= attenuation;
    float fog = clamp(1.0 - exp(-distanceToCamera * 0.052), 0.0, 1.0);
    color = mix(color, vec3(0.0, 0.47, 0.58), fog * 0.88);
    fragColor = vec4(color, 1.0);
}
