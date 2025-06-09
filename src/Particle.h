#pragma once

#include "raylib.h"
#include <vector>

struct Particle {
    Vector2 position;
    Vector2 velocity;
    float alpha;
    Color tint;
    float lifetime;
    float age = 0.0f;

    std::vector<Texture2D*> animationFrames;
    void setAnimationFrames(const std::vector<Texture2D>& textures);
    int currentFrame = 0;
    float animationSpeed = 0.1f;
    float animationTimer = 0.0f;

    bool active = false;

    void update(float deltaTime);
    void draw();
    void reset();
};
