#pragma once
#include "Behavior.h"
#include <memory>
#include <cstdint>

class Game;
class Sprite;

class ShootBurstBehavior : public Behavior
{
public:
    ShootBurstBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config, uint32_t burstCount, float burstDelay);
    void update(float deltaTime) override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> target;
    shootingConfig config;
    uint32_t burstCount;
    uint32_t shotsFired;
    float burstDelay;
};
