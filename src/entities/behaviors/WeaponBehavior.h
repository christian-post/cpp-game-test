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
    BOW = 5,
    HOOKSHOT = 6
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
    // projectiles
    std::string projectileKey;
    std::string projectileTrailEmitterKey;
    std::string projectileImpactEmitterKey;
    // ammunition
    bool needsAmmo;
    std::string ammoType;
    // hookshot config
    float maxHookshotRange = 80.0f;
    float hookshotPullSpeed = 160.0f; // pixels per second
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
    bool isInAimMode() const { return isNotched; } // for bows etc

    // hookshot specific fields
    enum class HookshotState
    {
        IDLE,
        FIRED,
        LATCHED_WORLD,
        LATCHED_ENEMY,
        RETRACTING
    };
    HookshotState hookshotState = HookshotState::IDLE;
    std::weak_ptr<Sprite> hookProjectile;
    Vector2 latchPosition = { 0.0f, 0.0f };
    std::weak_ptr<Sprite> latchedEnemy;
};
