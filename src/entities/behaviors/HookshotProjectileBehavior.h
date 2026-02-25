#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>

class Game;
class Sprite;

class HookshotProjectileBehavior : public Behavior
{
public:
    HookshotProjectileBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> owner, Vector2 direction, float maxRange);
    void update(float deltaTime) override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> owner;
    Vector2 direction;
    Vector2 startPosition = { 0.0f, 0.0f };
    float maxRange;
    float distanceTraveled = 0.0f;
    float hookSpeed = 200.0f;
    bool latched = false;
    bool retracting = false;
    void latchAt(Vector2 position, std::weak_ptr<Sprite> enemy = {});
};