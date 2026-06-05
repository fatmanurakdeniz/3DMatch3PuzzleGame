#version 330 core

in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;

out vec4 FragColor;

uniform sampler2D tex0;
uniform float useTexture;   // 1.0 = textured, 0.0 = flat color
uniform vec4 tileColor;     // base color or tint (white = no tint)

void main() {
    vec3 norm = normalize(Normal);

    // Directional light from upper-right-front
    vec3 lightDir = normalize(vec3(0.4, 0.8, 0.6));

    // Simple camera direction approximation in world space
    vec3 viewDir = normalize(vec3(3.5, 17.5, 9.8) - FragPos);

    // Ambient + diffuse
    float ambient = 0.22;
    float diff = max(dot(norm, lightDir), 0.0);

    // Blinn-Phong specular
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);

    // Rim light helps rounded objects read as 3D
    float rim = pow(1.0 - max(dot(norm, viewDir), 0.0), 2.2);

    vec4 baseColor;
    if (useTexture > 0.5) {
        baseColor = texture(tex0, TexCoord);
        if (baseColor.a < 0.1) discard;

        // textured path: texture tinted by tileColor
        baseColor.rgb *= tileColor.rgb;
        baseColor.a *= tileColor.a;
    } else {
        // flat-color path: tileColor is already the base color
        baseColor = tileColor;
    }

    float lighting = ambient + 0.75 * diff;
    vec3 color = baseColor.rgb * lighting;

    // add highlights
    color += vec3(1.0) * (0.22 * spec);
    color += baseColor.rgb * (0.18 * rim);

    FragColor = vec4(color, baseColor.a);
}