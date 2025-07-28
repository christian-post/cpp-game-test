#include "CircleOverlay.h"
#include <cassert>
#include <cstdio>


void DrawLightOverlay(Texture2D& texture, const Shader& shader, Light* lights, int lightCount, float screenW, float screenH) {
    Vector2 centers[MAX_LIGHTS];
    float radii[MAX_LIGHTS];
    for (int i = 0; i < lightCount; i++) {
        centers[i] = lights[i].center;
        radii[i] = lights[i].radius;
    }
    SetShaderValue(shader, GetShaderLocation(shader, "u_lightCount"), &lightCount, SHADER_UNIFORM_INT);
    SetShaderValueV(shader, GetShaderLocation(shader, "u_centers"), centers, SHADER_UNIFORM_VEC2, lightCount);
    SetShaderValueV(shader, GetShaderLocation(shader, "u_radii"), radii, SHADER_UNIFORM_FLOAT, lightCount);
    SetShaderValue(shader, GetShaderLocation(shader, "screenWidth"), &screenW, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "screenHeight"), &screenH, SHADER_UNIFORM_FLOAT);

    BeginShaderMode(shader);
    Rectangle source = { 0.0f, 0.0f, screenW, screenH };
    Rectangle dest = { 0.0f, 0.0f, screenW, screenH };
    DrawTexturePro(texture, source, dest, { 0.0f, 0.0f }, 0.0f, WHITE);
    EndShaderMode();
}
