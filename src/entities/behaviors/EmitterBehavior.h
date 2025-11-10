#pragma once
#include "Behavior.h"
#include <memory>

class Game;
class Sprite;
struct Emitter;

class EmitterBehavior : public Behavior {
public:
    EmitterBehavior(Game& game, std::shared_ptr<Sprite> self, std::unique_ptr<Emitter> emitter);
    void update(float deltaTime) override;
    void reset() override;
    void onDeactivate() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    Emitter* emitter = nullptr;
};
