#pragma once
#include "Behavior.h"
#include "raylib.h"
#include <memory>
#include <optional>

class Game;
class Sprite;
struct Emitter;

class ProjectileBehavior : public Behavior {
public:
    ProjectileBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, bool steer = false, std::optional<Vector2> customDirection = std::nullopt, std::string trailEmitterKey = "", std::string impactEmitterKey = "");
    void update(float deltaTime) override;
    void draw() override;
    std::string trailEmitterKey = "";
    std::string impactEmitterKey = "";

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> target;
    bool steer;
    Vector2 direction = { 0.0f, 0.0f };
    Emitter* impactEmitter = nullptr;
    float lifetimeAfterImpact = 0.0f;
    void createImpactEffect(std::shared_ptr<Sprite> s);
    void stopProjectile(std::shared_ptr<Sprite> s);
};
