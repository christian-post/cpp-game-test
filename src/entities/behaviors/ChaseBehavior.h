#pragma once
#include "Behavior.h"
#include <memory>

class Game;
class Sprite;

class ChaseBehavior : public Behavior
{
public:
    ChaseBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, float minDist);
    void update(float deltaTime) override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> other;
    float minDist;
    bool isChasing = false;
};
