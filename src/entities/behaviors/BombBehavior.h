#pragma once
#include "Behavior.h"
#include <memory>

class Game;
class Sprite;

struct bombConfig
{
    float fuseTime = 2.0f;
    float explosionRadius = 48.0f;
    float explosionDuration = 0.1f;
    uint32_t damage = 10;
    std::string emitterKey = "exampleBurst";
    float flashSpeedMin = 1.0f; // flashes per second
    float flashSpeedMax = 5.0f;
};

class BombBehavior : public Behavior
{
public:
    BombBehavior(Game& game, std::shared_ptr<Sprite> self, bombConfig config = {});
    void update(float deltaTime) override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    bombConfig config;
    bool exploded = false;
    float explosionTimer = 0.0f;
    std::weak_ptr<Sprite> explosionSprite;
};