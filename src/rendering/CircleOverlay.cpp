#include "CircleOverlay.h"
#include <cassert>
#include <cstdio>


void DrawLightOverlay(Texture2D& texture, const Shader& shader, Light* lights, int lightCount, float screenW, float screenH) {
    Vector2 centers[MAX_LIGHTS];
    float radii[MAX_LIGHTS];
    for (int i = 0; i < lightCount; i++) {
        if (lights[i].active) {
            centers[i] = lights[i].center;
        }
        else {
            centers[i] = { -100.0f, -100.0f };
        }
        radii[i] = lights[i].radius;
    }
    float time = GetTime();
    SetShaderValue(shader, GetShaderLocation(shader, "time"), &time, SHADER_UNIFORM_FLOAT);
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

void DrawVignette(Texture2D& texture, const Shader& shader, float intensity, float softness, float screenW, float screenH) {
    // Set shader uniforms (matching the light shader pattern)
    SetShaderValue(shader, GetShaderLocation(shader, "screenWidth"), &screenW, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "screenHeight"), &screenH, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "vignetteIntensity"), &intensity, SHADER_UNIFORM_FLOAT);
    SetShaderValue(shader, GetShaderLocation(shader, "vignetteSoftness"), &softness, SHADER_UNIFORM_FLOAT);

    // Apply shader and draw texture
    BeginShaderMode(shader);
    Rectangle source = { 0.0f, 0.0f, screenW, screenH };
    Rectangle dest = { 0.0f, 0.0f, screenW, screenH };
    DrawTexturePro(texture, source, dest, { 0.0f, 0.0f }, 0.0f, WHITE);
    EndShaderMode();
}