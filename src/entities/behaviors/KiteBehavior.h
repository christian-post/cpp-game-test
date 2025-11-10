#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>

class Game;
class Sprite;

class KiteBehavior : public Behavior {
public:
    KiteBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, float orbitDistance, float moveSpeed);
    void update(float deltaTime) override;
    void draw() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> target;
    Vector2 desiredPos = { 0.0f, 0.0f };
    float orbitDistance;
    float moveSpeed;
    float orbitAngle;
};
