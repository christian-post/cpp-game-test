#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>
#include <cstdint>

class Game;
class Sprite;

class LungeBehavior : public Behavior {
public:
    LungeBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, float lungeSpeed, uint32_t jumpForce);
    void update(float deltaTime) override;
    void draw() override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> target;
    float lungeSpeed;
    uint32_t jumpForce;
    Vector2 lungeDirection;
    bool hasLunged = false;
    bool isAirborne = false;
    float originalSpeed;
};
