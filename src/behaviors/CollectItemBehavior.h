#pragma once
#include "Behavior.h"
#include <memory>
#include <string>
#include <cstdint>

class Game;
class Sprite;

class CollectItemBehavior : public Behavior {
public:
    CollectItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const std::string& name, uint32_t amount);
    void update(float deltaTime) override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> other;
    std::string name;
    uint32_t amount;
    uint32_t state = 0;
    float displayDuration = 2.0f;
};
