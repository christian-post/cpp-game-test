#pragma once
#include "raylib.h"
#include <vector>
#include "json.hpp"

enum class ParticleType {
    Texture,
    Circle,
    Square
};

enum class EasingType {
    None,
    QuadIn,
    QuadOut,
    QuadInOut,
    CubicIn,
    CubicOut,
    CubicInOut
};

struct Particle {
    Particle();
    void update(float deltaTime);
    void draw();
    void reset();
    void fromJSON(const nlohmann::json& data, const nlohmann::json& defaultData);

    ParticleType type = ParticleType::Texture;
    float primitiveSize = 5.0f;

    Vector2 position = { 0.0f, 0.0f };
    Vector2 velocity = { 0.0f, 0.0f }; // dynamic velocity
    Vector2 initialVelocity = { 0.0f, 0.0f }; // save vel before easing is applied
    Vector2 gravity = { 0.0f, 0.0f };
    EasingType velocityEasing = EasingType::None;
    float alpha = 1.0f;
    float startAlpha = 1.0f;
    float endAlpha = 0.0f;
    Color tint = WHITE;
    float lifetime = 1.0f;
    float age = 0.0f;
    float startSize = 1.0f; // fixed
    float endSize = 1.0f; // fixed
    float size = 1.0f; // changes; used only internally

    std::vector<Texture2D*> animationFrames;
    void setAnimationFrames(const std::vector<Texture2D>& textures);
    int currentFrame = 0;
    float animationSpeed = 0.1f;
    float animationTimer = 0.0f;
    bool active = false;
};
