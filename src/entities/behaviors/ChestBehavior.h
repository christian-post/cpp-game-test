#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>
#include <string>
#include <cstdint>

class Game;
class Sprite;

class ChestBehavior : public Behavior {
public:
    ChestBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, const std::string& itemName, uint32_t itemAmount);
    void update(float deltaTime) override;
    void draw() override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> player;
    std::string itemName;
    uint32_t itemAmount;
    bool triggered = false;
    bool collided = false;
    bool showItem = false;
    Rectangle interactionRect = Rectangle{ 0.0f, 0.0f ,0.0f, 0.0f };
};
