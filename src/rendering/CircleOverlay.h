#pragma once
#include "raylib.h"

constexpr int MAX_LIGHTS = 10;

struct Light {
    Vector2 center;
    float radius;
    bool active = false;
};

void DrawLightOverlay(Texture2D& texture, const Shader& shader, Light* lights, int lightCount, float screenW, float screenH);

void DrawVignette(Texture2D& texture, const Shader& shader, float intensity, float softness, float screenW, float screenHH);

void DrawLowHealthEffect(Texture2D& texture, const Shader& shader, float frequency, float intensity, float softness, float screenW, float screenH);

void DrawTransition(Texture2D& texture, const Shader& shader, Vector2 center, float screenW, float screenH, float startTime, float duration, int opening);