#pragma once
#include "Behavior.h"
#include <memory>

class Game;
class Sprite;

class ShootBehavior : public Behavior
{
public:
    ShootBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config);
    void update(float deltaTime) override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> target;
    shootingConfig config;
    float interval = 0.0f;
};
