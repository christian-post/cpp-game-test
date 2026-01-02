#pragma once
#include "Behavior.h"
#include <memory>

class Game;
class Sprite;

enum weaponType {
    SWING = 0,
    POKE = 1,
    SHOOT = 2,
    WHACK = 3,
    HOLD = 4,
    BOW = 5
};

struct weaponData {
    // encapsulates the data from weapons.json
    weaponType type;
    uint32_t damage;
    float posOffsetX;
    float posOffsetY;
    float HurtboxOffsetX;
    float HurtboxOffsetY;
    float HurtboxWidth;
    float HurtboxHeight;
    float lifetime;
    std::string soundKey;
    std::string projectileKey;
    std::string projectileTrailEmitterKey;
    std::string projectileImpactEmitterKey;
    // optional callbacks 
    std::function<void()> onCreate;
    std::function<void()> onDestroy;
};


class WeaponBehavior : public Behavior
{
public:
    WeaponBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> owner, weaponData data, size_t slot);
    void update(float deltaTime) override;
    void draw() override;
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
    bool isNotched = false;
    direction notchedDirection = RIGHT;
    Vector2 aimDirection = { 1.0f, 0.0f };
    bool isInAimMode() const { return isNotched; } // for bows
};
