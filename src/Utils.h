#pragma once
#include "raylib.h"
#include "raymath.h"
#include "Sprite.h"
#include <iostream>

inline Vector2 GetRectCenter(Rectangle rect) {
    return { rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f };
}


template <typename T>
inline void Log(T message) {
    std::cout << "INFO: " << message << std::endl;
}

inline void Log(const Vector2& v) {
    std::cout << "INFO: (" << v.x << ", " << v.y << ")" << std::endl;
}


inline bool isPathClear(const Rectangle& currentRect, Vector2 targetPos, const std::vector<std::unique_ptr<Rectangle>>& walls) {
    Rectangle sweptRect = {
        std::min(currentRect.x, targetPos.x),
        std::min(currentRect.y, targetPos.y),
        fabsf(targetPos.x - currentRect.x) + currentRect.width,
        fabsf(targetPos.y - currentRect.y) + currentRect.height
    };

    for (const auto& wall : walls) {
        if (CheckCollisionRecs(sweptRect, *wall)) {
            return false;
        }
    }
    return true;
}

inline void applyKnockback(Sprite& sourceSprite, Sprite& targetSprite, float strength) {
    Vector2 sourceCenter = GetRectCenter(sourceSprite.hurtbox);
    Vector2 targetCenter = GetRectCenter(targetSprite.rect);
    Vector2 direction = Vector2Normalize(Vector2Subtract(targetCenter, sourceCenter));
    targetSprite.vel = Vector2Scale(direction, strength);
}