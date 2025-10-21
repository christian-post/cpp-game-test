#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>

class Sprite;

class RandomWalkBehavior : public Behavior {
public:
    RandomWalkBehavior(std::shared_ptr<Sprite> self);
    void update(float deltaTime) override;
    void reset() override;

private:
    std::weak_ptr<Sprite> self;
    float waitTime = 0.0f;
    Vector2 walkTarget = { 0.0f, 0.0f };
    bool hasWalkTarget = false;
};
