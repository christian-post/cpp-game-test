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
    ProjectileBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, bool steer = false, std::optional<Vector2> customDirection = std::nullopt);
    void update(float deltaTime) override;
    void draw() override;

private:
    Game& game;
    std::weak_ptr<Sprite> self;
    std::weak_ptr<Sprite> target;
    bool steer;
    Vector2 direction = { 0.0f, 0.0f };
    Emitter* impactEmitter = nullptr;
    void createImpactEffect(std::shared_ptr<Sprite> s);
    void stopProjectile(std::shared_ptr<Sprite> s);
};
