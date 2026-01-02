#pragma once
#include "Behavior.h"
#include <memory>
#include <string>
#include <cstdint>

class Game;
class Sprite;

class TradeItemBehavior : public Behavior
{
public:
    TradeItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::string name, uint32_t price);
    void update(float deltaTime) override;
    void draw() override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> player;
    std::string name;
    uint32_t price;
    bool triggered = false;
    bool collided = false;
};
