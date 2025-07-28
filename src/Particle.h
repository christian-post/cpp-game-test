#pragma once
#include "raylib.h"
#include <vector>
#include "json.hpp"

struct Particle {
    Vector2 position = { 0.0f, 0.0f };
    Vector2 velocity = { 0.0f, 0.0f };
    float alpha = 1.0f;
    float startAlpha = 1.0f;
    float endAlpha = 0.0f;
    Color tint = WHITE;
    float lifetime = 1.0f;
    float age = 0.0f;
    float startSize = 1.0f;
    float endSize = 1.0f;
    float size = 1.0f;

    std::vector<Texture2D*> animationFrames;
    void setAnimationFrames(const std::vector<Texture2D>& textures);
    int currentFrame = 0;
    float animationSpeed = 0.1f;
    float animationTimer = 0.0f;

    bool active = false;

    Particle();
    void update(float deltaTime);
    void draw();
    void reset();
    void fromData(nlohmann::json& data);
};
