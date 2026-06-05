#version 330 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D tex0;
uniform float useTexture;   // 1.0 = textured tile, 0.0 = flat color
uniform vec4 tileColor;

void main() {
    if (useTexture > 0.5) {
        vec4 texColor = texture(tex0, TexCoord);

        // For PNG with real alpha (blue.png): discard transparent pixels
        if (texColor.a < 0.1) discard;

        // For JPEG/RGB-PNG with black background: discard nearly pure-black pixels only
        // (shell highlights and colors are bright enough; only discard very dark bg)
        float maxC = max(texColor.r, max(texColor.g, texColor.b));
        if (maxC < 0.06) discard;

        // Smooth edge at the black boundary (soft alpha based on brightness)
        float edgeAlpha = smoothstep(0.04, 0.14, maxC);
        float finalAlpha = min(texColor.a, edgeAlpha);

        FragColor = vec4(texColor.rgb * tileColor.rgb, finalAlpha * tileColor.a);

    } else {
        // Flat rounded-rect for obstacles and cell backgrounds
        vec2 uv = TexCoord;
        float r = 0.13;
        vec2 d = abs(uv - 0.5) - (0.5 - r);
        float dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;

        if (dist > 0.01) discard;

        float alpha = tileColor.a * (1.0 - smoothstep(-0.01, 0.01, dist));
        float highlight = 0.20 * (1.0 - smoothstep(0.0, 0.38, length(uv - vec2(0.28, 0.22))));
        float depth = 0.88 + 0.12 * (1.0 - uv.y);

        vec3 finalColor = clamp(tileColor.rgb * depth + tileColor.rgb * highlight, 0.0, 1.0);
        FragColor = vec4(finalColor, alpha);
    }
}
