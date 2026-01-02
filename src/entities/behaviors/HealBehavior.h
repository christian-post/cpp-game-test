#pragma once
#include "Behavior.h"
#include <memory>
#include <cstdint>

class Game;
class Sprite;

class HealBehavior : public Behavior
{
public:
    HealBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, uint32_t amount);
    void update(float deltaTime) override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> other;
    uint32_t amount;
};
