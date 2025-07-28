#pragma once
#include "raylib.h"

constexpr int MAX_LIGHTS = 10;

struct Light {
    Vector2 center;
    float radius;
    bool active = false;
};

void DrawLightOverlay(Texture2D& texture, const Shader& shader, Light* lights, int lightCount, float screenW, float screenH);
