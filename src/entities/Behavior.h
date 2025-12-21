#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>
#include <cstdint>

#include "json.hpp"
#include "raylib.h"


class Game;
class Sprite;
struct Emitter;
struct Particle;

enum direction {
    RIGHT,
    UP,
    LEFT,
    DOWN
};

struct shootingConfig {
    // a copy is passed to ShootBehavior's constructor
    // sprite (main projectile)
    std::string projectileKey = "sprite_default";
    std::string projectileTrailEmitterKey = "";
    std::string projectileImpactEmitterKey = "";
    std::string emitterKey = ""; // empty = use projectile texture, non-empty = load from particles.json
    std::string sound = "powerUp1"; // just a default sound to indicate a missing override
    uint32_t damage = 0;
    float speed = 1.0f;
    float frameTime = 0.1f;
    float hitboxSize = 8.0f;
    float shootInterval = 1.0f; // seconds
    // particle effect (in addition to the projectile)
};

struct TeleportEvent {
    std::string targetMap;
    Vector2 targetPos;
};


// Behavior base class
class Behavior {
public:
    virtual ~Behavior() = default;
    virtual void update(float deltaTime) = 0;
    virtual void draw() {};
    virtual void onDeactivate() {}

    // Reset the behavior to its initial state
    // Override this in child classes that need to reset additional state beyond timer
    virtual void reset() {
        done = false;
        timer = 0.0f;
    }

    bool done = false;

protected:
    float timer = 0.0f; // Base timer that counts UP - tracks elapsed time since behavior started/reset
};


// Factory function to create behaviors from JSON data
std::unique_ptr<Behavior> createBehaviorFromJSON(
    Game& game,
    std::shared_ptr<Sprite> sprite,
    const std::string& behaviorKey,
    const nlohmann::json& behaviorData
);

// puts all behaviors into the Sprite instance
void addBehaviorsToSprite(Game& game, std::shared_ptr<Sprite> sprite, const std::vector<std::string>& behaviors, const nlohmann::json& behaviorData);