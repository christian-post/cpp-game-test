#version 330
#define MAX_LIGHTS 10

uniform int u_lightCount;
uniform vec2 u_centers[MAX_LIGHTS];
uniform float u_radii[MAX_LIGHTS];
uniform float screenWidth;
uniform float screenHeight;

in vec2 fragTexCoord;
out vec4 finalColor;

void main() {
    vec2 fragCoord = fragTexCoord * vec2(screenWidth, screenHeight);
    float alpha = 1.0;

    for (int i = 0; i < u_lightCount; i++) {
        float dist = distance(fragCoord, u_centers[i]);
        float falloff = max(u_radii[i] * 0.75, 1.0);
        float fade = smoothstep(u_radii[i], u_radii[i] - falloff, dist);
        alpha *= 1.0 - fade;
    }

    finalColor = vec4(0.0, 0.0, 0.0, alpha * 0.95);
}
