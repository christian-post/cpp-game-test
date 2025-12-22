#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>
#include <string>

class Game;
class Sprite;

class StairsBehavior : public Behavior {
public:
    StairsBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const int level);
    void update(float deltaTime) override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> other;
    const int levelChange = 0;
};