#include "Utils.h"
#include "Sprite.h"
#include "TileMap.h"
#include "Game.h"
#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <cstdlib>
#include <chrono>
#include <ctime>

Vector2 GetRectCenter(Rectangle rect) 
{
    return { rect.x + rect.width / 2.0f, rect.y + rect.height / 2.0f };
}

bool isPathClear(const Rectangle& currentRect, Vector2 targetPos, const std::vector<std::unique_ptr<CollisionObject>>& walls, int spriteLayer) 
{
    Rectangle sweptRect = {
        std::min(currentRect.x, targetPos.x),
        std::min(currentRect.y, targetPos.y),
        fabsf(targetPos.x - currentRect.x) + currentRect.width,
        fabsf(targetPos.y - currentRect.y) + currentRect.height
    };
    for (const auto& wall : walls)
    {
        // check if wall and sprite are on similar collision layers
        if (wall->layer == spriteLayer && CheckCollisionRecs(sweptRect, wall->getRect()))
            return false;
    }
    return true;
}

void applyKnockback(Sprite& sourceSprite, Sprite& targetSprite, float strength) 
{
    Vector2 sourceCenter = GetRectCenter(sourceSprite.hurtbox);
    Vector2 targetCenter = GetRectCenter(targetSprite.rect);
    Vector2 direction = Vector2Normalize(Vector2Subtract(targetCenter, sourceCenter));
    targetSprite.vel = Vector2Scale(direction, strength);
}

std::vector<std::string> splitCSV(const std::string& input) 
{
    std::vector<std::string> result;
    std::istringstream ss(input);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        result.push_back(token);
    }
    return result;
}

float getRandomFloat(float min, float max) 
{
    return min + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / (max - min)));
}

void TraceLogLong(int logLevel, const std::string& message)
{
    // circumvents the size limitation of raylib's logging
    // TODO doesn't work?
    const size_t chunkSize = 500;

    if (message.length() <= chunkSize)
    {
        TraceLog(logLevel, message.c_str());
        return;
    }

    for (size_t i = 0; i < message.length(); i += chunkSize)
    {
        std::string chunk = message.substr(i, chunkSize);
        TraceLog(logLevel, chunk.c_str());
    }
}

void CameraShake::start(float dur, float xMag, float yMag) 
{
    duration = dur;
    baseDuration = dur;
    xMagnitude = xMag;
    yMagnitude = yMag;
}

void CameraShake::update(float deltaTime) 
{
    duration -= deltaTime;
    if (duration <= 0.0f)
    {
        duration = 0.0f;
        xMagnitude = 0.0f;
        yMagnitude = 0.0f;
    }
}

Vector2 CameraShake::apply(const Vector2& baseTarget) const 
{
    if (duration > 0.0f && baseDuration > 0.0f)
    {
        float t = 1.0f - (duration / baseDuration);
        float strength = sinf(t * 3.14159f);
        return {
            baseTarget.x + strength * xMagnitude,
            baseTarget.y - strength * yMagnitude
        };
    }
    return baseTarget;
}

std::vector<std::string> listJSONFiles(const std::string& path) 
{
    std::vector<std::string> jsonFiles;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.path().extension() == ".json")
            jsonFiles.push_back(entry.path().string());
    }
    return jsonFiles;
}

void mergeJson(nlohmann::json& base, const nlohmann::json & override)
{
    for (const auto& item : override.items())
    {
        const std::string& key = item.key();
        const nlohmann::json& value = item.value();
        if (base.contains(key) && base[key].is_object() && value.is_object())
            mergeJson(base[key], value);
        else
            base[key] = value;
    }
}

std::vector<std::string> listFiles(const std::string& path) 
{
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(path))
    {
        if (entry.is_regular_file())
            files.push_back(entry.path().string());
    }
    return files;
}

std::string GetBaseName(const std::string& path) 
{
    // returns the filename without parents and extension
    std::filesystem::path p(path);
    return p.stem().string();  
}

std::string GetLastWriteTime(const std::string& path) 
{
    auto ftime = std::filesystem::last_write_time(path);
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - decltype(ftime)::clock::now() + std::chrono::system_clock::now()
    );
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
    std::tm timeinfo;
    char buf[26];
#ifdef _WIN32
    localtime_s(&timeinfo, &cftime);
    asctime_s(buf, sizeof(buf), &timeinfo);
#else
    localtime_r(&cftime, &timeinfo);
    asctime_r(&timeinfo, buf);
#endif
    return std::string(buf);
}

void resolveAxisX(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle) 
{
    if (!sprite->isColliding || !CheckCollisionRecs(sprite->rect, obstacle))
        return;

    float spriteCenterX = sprite->rect.x + sprite->rect.width * 0.5f;
    float obstacleCenterX = obstacle.x + obstacle.width * 0.5f;

    if (spriteCenterX < obstacleCenterX)
        sprite->position.x = obstacle.x - sprite->rect.width;
    else
        sprite->position.x = obstacle.x + obstacle.width;

    sprite->vel.x = 0.0f;
    sprite->rect.x = sprite->position.x + sprite->hitboxOffset.x;
}

void resolveAxisY(const std::shared_ptr<Sprite>& sprite, const Rectangle& obstacle) 
{
    if (!sprite->isColliding || !CheckCollisionRecs(sprite->rect, obstacle))
        return;

    float spriteCenterY = sprite->rect.y + sprite->rect.height * 0.5f;
    float obstacleCenterY = obstacle.y + obstacle.height * 0.5f;

    if (spriteCenterY < obstacleCenterY)
        sprite->position.y = obstacle.y - sprite->rect.height;
    else
        sprite->position.y = obstacle.y + obstacle.height + 0.1f;

    sprite->vel.y = 0.0f;
    sprite->rect.y = sprite->position.y + sprite->hitboxOffset.y;
}


bool isSubset(const std::unordered_set<std::string>& subset, const std::unordered_set<std::string>& superset) 
{
    for (const auto& item : subset)
    {
        if (superset.find(item) == superset.end())
            return false;
    }
    return true;
}

void DrawTextWithSprites(const char* text, int x, int y, int fontSize, Color color, Game& game)
{
    std::string str(text);
    int currentX = x;
    int currentY = y;
    size_t pos = 0;

    while (pos < str.length())
    {
        // check for newline
        size_t newlinePos = str.find('\n', pos);
        size_t texStart = str.find("[TEX:", pos);

        // handle newline before token
        if (newlinePos != std::string::npos && (texStart == std::string::npos || newlinePos < texStart))
        {
            // draw text up to newline
            std::string beforeNewline = str.substr(pos, newlinePos - pos);
            DrawText(beforeNewline.c_str(), currentX, currentY, fontSize, color);

            // move to next line
            currentY += fontSize + 2;
            currentX = x;
            pos = newlinePos + 1;
            continue;
        }

        // handle token
        if (texStart != std::string::npos && texStart >= pos)
        {
            // draw text before token
            if (texStart > pos)
            {
                std::string before = str.substr(pos, texStart - pos);
                DrawText(before.c_str(), currentX, currentY, fontSize, color);
                currentX += MeasureText(before.c_str(), fontSize);
            }

            // find token end
            size_t texEnd = str.find("]", texStart);

            if (texEnd == std::string::npos)
            {
                // incomplete token, draw as text and stop
                std::string partial = str.substr(texStart);
                DrawText(partial.c_str(), currentX, currentY, fontSize, color);
                return;
            }

            // parse token: [TEX:texture_key:frame_index]
            std::string token = str.substr(texStart + 5, texEnd - texStart - 5);
            size_t colonPos = token.find(":");

            if (colonPos != std::string::npos)
            {
                std::string textureKey = token.substr(0, colonPos);
                int frameIndex = std::stoi(token.substr(colonPos + 1));

                const auto& texture = game.loader.getTextures(textureKey)[frameIndex];
                // center sprite vertically with text
                DrawTexture(texture, currentX, currentY + (fontSize - 16) / 2, WHITE);
                currentX += 16;
            }

            pos = texEnd + 1;
        }
        else
        {
            // no more tokens or newlines, draw rest of text
            std::string rest = str.substr(pos);
            DrawText(rest.c_str(), currentX, currentY, fontSize, color);
            break;
        }
    }
}

int MeasureTextWithSprites(const char* text, int fontSize, Game& game)
{
    std::string str(text);
    size_t texStart = str.find("[TEX:");

    if (texStart == std::string::npos)
        return MeasureText(text, fontSize);

    int totalWidth = 0;

    // measure text before token
    if (texStart > 0)
    {
        std::string before = str.substr(0, texStart);
        totalWidth += MeasureText(before.c_str(), fontSize);
    }

    // add sprite width
    size_t texEnd = str.find("]", texStart);
    if (texEnd != std::string::npos)
    {
        totalWidth += 16; // sprite width

        // measure text after token
        if (texEnd + 1 < str.length())
        {
            std::string after = str.substr(texEnd + 1);
            totalWidth += MeasureTextWithSprites(after.c_str(), fontSize, game);
        }
    }

    return totalWidth;
}
