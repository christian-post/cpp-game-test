#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>

class Game;
class Sprite;

class OpenLockBehavior : public Behavior {
public:
    OpenLockBehavior(Game& game, std::shared_ptr<Sprite> door, std::shared_ptr<Sprite> player, const int triggerKey);
    void update(float deltaTime) override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> door;
    std::weak_ptr<Sprite> player;
    const int triggerKey;
    bool triggered = false;
    bool collided = true;
    Rectangle interactionRect = Rectangle{ 0.0f, 0.0f ,0.0f, 0.0f };
};
