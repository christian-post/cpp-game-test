#pragma once
#include "raylib.h"
#include <string>
#include <vector>
#include <memory>
#include <cstdarg>
#include <filesystem>
#include "json.hpp"
#include <unordered_set>

class Sprite;
struct CollisionObject;

Vector2 GetRectCenter(Rectangle rect);
bool isPathClear(const Rectangle& currentRect, Vector2 targetPos, const std::vector<std::unique_ptr<CollisionObject>>& walls, int spriteLayer = 0);
void applyKnockback(Sprite& sourceSprite, Sprite& targetSprite, float strength);
std::vector<std::string> splitCSV(const std::string& input);
float getRandomFloat(float min, float max);
void TraceLogLong(int logLevel, const std::string& message);
std::vector<std::string> listJSONFiles(const std::string& path);
void mergeJson(nlohmann::json& base, const nlohmann::json & override);
std::vector<std::string> listFiles(const std::string& path);
std::string GetBaseName(const std::string& path);
std::string GetLastWriteTime(const std::string& path);
void resolveAxisX(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle);
void resolveAxisY(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle);
bool isSubset(const std::unordered_set<std::string>& subset, const std::unordered_set<std::string>& superset);

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

inline std::string format(const char* fmt, ...) {
    // C-string formatting function
    char buffer[256]; // adjust size if needed
    va_list args;
    va_start(args, fmt);
    std::vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    return std::string(buffer);
}