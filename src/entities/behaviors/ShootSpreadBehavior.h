#pragma once
#include "Behavior.h"
#include <memory>
#include <cstdint>

class Game;
class Sprite;

class ShootSpreadBehavior : public Behavior {
public:
    ShootSpreadBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config, uint32_t projectileCount, float spreadAngle);
    void update(float deltaTime) override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> target;
    shootingConfig config;
    uint32_t projectileCount;
    float spreadAngle;
    bool hasFired;
};
