#pragma once
#include "raylib.h"    
#include <string>       
#include <vector>         
#include <memory>   

class Sprite;

Vector2 GetRectCenter(Rectangle rect);
bool isPathClear(const Rectangle& currentRect, Vector2 targetPos, const std::vector<std::unique_ptr<Rectangle>>& walls);
void applyKnockback(Sprite& sourceSprite, Sprite& targetSprite, float strength);
std::vector<std::string> splitCSV(const std::string& input);
float getRandomFloat(float min, float max);

struct CameraShake {
    float duration = 0.0f;
    float baseDuration = 0.0f;
    float xMagnitude = 0.0f;
    float yMagnitude = 0.0f;

    void start(float dur, float xMag, float yMag);
    void update(float deltaTime);
    Vector2 apply(const Vector2& baseTarget) const;
    bool isActive() const { return duration > 0.0f; }
};

