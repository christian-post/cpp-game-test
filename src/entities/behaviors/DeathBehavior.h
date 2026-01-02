#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>

class Game;
class Sprite;

class DeathBehavior : public Behavior
{
public:
    DeathBehavior(Game& game, std::shared_ptr<Sprite> self, float lifetime);
    void update(float deltaTime) override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    float lifetime;
    float maxLifetime;
    const Shader* shader = nullptr;
};
