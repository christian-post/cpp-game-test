#pragma once
#include "Behavior.h"
#include <memory>

class Game;
class Sprite;

class WeaponBehavior : public Behavior {
public:
    WeaponBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> owner, weaponData data, size_t slot);
    void update(float deltaTime) override;
    void reset() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> owner;
    size_t slot;
    static const int controlBindings[2];
    weaponData data;
    float lifetime;
    float originalLifetime;
    bool switchedOn = false;
    bool shaken = false;
};
