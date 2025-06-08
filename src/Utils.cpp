#include "Utils.h"
#include "Sprite.h"              
#include "raylib.h"
#include "raymath.h"
#include <algorithm>             
#include <cmath>                 
#include <sstream>                
#include <cstdlib>       

Vector2 GetRectCenter(Rectangle rect) {
    return { rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f };
}

bool isPathClear(const Rectangle& currentRect, Vector2 targetPos, const std::vector<std::unique_ptr<Rectangle>>& walls) {
    Rectangle sweptRect = {
        std::min(currentRect.x, targetPos.x),
        std::min(currentRect.y, targetPos.y),
        fabsf(targetPos.x - currentRect.x) + currentRect.width,
        fabsf(targetPos.y - currentRect.y) + currentRect.height
    };
    for (const auto& wall : walls) {
        if (CheckCollisionRecs(sweptRect, *wall)) return false;
    }
    return true;
}

void applyKnockback(Sprite& sourceSprite, Sprite& targetSprite, float strength) {
    Vector2 sourceCenter = GetRectCenter(sourceSprite.hurtbox);
    Vector2 targetCenter = GetRectCenter(targetSprite.rect);
    Vector2 direction = Vector2Normalize(Vector2Subtract(targetCenter, sourceCenter));
    targetSprite.vel = Vector2Scale(direction, strength);
}

std::vector<std::string> splitCSV(const std::string& input) {
    std::vector<std::string> result;
    std::istringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ',')) result.push_back(token);
    return result;
}

float getRandomFloat(float min, float max) {
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void CameraShake::start(float dur, float xMag, float yMag) {
    duration = dur;
    baseDuration = dur;
    xMagnitude = xMag;
    yMagnitude = yMag;
}

void CameraShake::update(float deltaTime) {
    duration -= deltaTime;
    if (duration <= 0.0f) {
        duration = 0.0f;
        xMagnitude = 0.0f;
        yMagnitude = 0.0f;
    }
}

Vector2 CameraShake::apply(const Vector2& baseTarget) const {
    if (duration > 0.0f && baseDuration > 0.0f) {
        float t = 1.0f - (duration / baseDuration);
        float strength = sinf(t * 3.14159f);
        return {
            baseTarget.x + strength * xMagnitude,
            baseTarget.y - strength * yMagnitude
        };
    }
    return baseTarget;
}
