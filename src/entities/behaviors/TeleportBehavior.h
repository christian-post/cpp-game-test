#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>
#include <string>

class Game;
class Sprite;

class TeleportBehavior : public Behavior
{
public:
    TeleportBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const std::string& targetWorld, size_t targetLevel, size_t targetIndex, Vector2 targetPos);
    void update(float deltaTime) override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> other;
    std::string targetWorld;
    size_t targetLevel;
    size_t targetIndex;
    Vector2 targetPos;
};
