#pragma once

#include "raylib.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include "Behavior.h"
#include <cstdint>


class Game;


struct ShaderState {
    const Shader* shader = nullptr;
    float time = 0.0f;
    float duration = 2.0f;
    int flipX = 0;
};


class Sprite {
public:
    Game& game;
    Rectangle rect; // hitbox for collision
    Rectangle hurtbox; // hurtbox for attacks
    Vector2 hurtboxOffset = { 0.0f, 0.0f };
    std::unordered_map<std::string, std::vector<Texture2D>> frames;
    std::string spriteName;
    int currentFrame = 0;
    bool doesAnimate = true;
    bool visible = true;
    bool persistent = false; // controls whether the sprite survives between map changes
    std::string currentTextureKey = "default";
    std::optional<ShaderState> activeShader = std::nullopt;
    direction lastDirection = RIGHT;
    float rotationAngle = 0.0f;
    float frameTime = 0.12f; // animation speed
    float elapsedTime = 0.0f;

    // physics
    bool isColliding = true;
    float speed = 20.0f; // default movement speed
    Vector2 acc;
    Vector2 vel;
    float friction = 0.8f;
    Vector2 position; // position exists independently of rect to allow for subpixel accurate movement
    bool staticCollision = false; // behaves like a wall

    // gameplay variables
    uint32_t health; // current health
    uint32_t maxHealth;
    float iFrameTimer = 0.0f; // duration of invincibility (s)
    bool canHurtPlayer = false;
    uint32_t damage = 0;
    bool dying = false; // flag for the death animation
    
    Sprite(Game& game, float x, float y, float w, float h, const std::string& spriteName);
    ~Sprite();
    void setTextures(std::initializer_list<std::string> keys);
    void animate(float deltaTime);
    void setHurtbox(float x = -1.0f, float y = -1.0f, float width = -1.0f, float height = -1.0f, bool center = false);
    void getControls();
    void update(float deltaTime);
    void draw();
    void moveTo(float x, float y);
    bool isMarkedForDeletion() const { return markedForDeletion; }
    void markForDeletion() { markedForDeletion = true; }

    // behavior methods
    void addBehavior(std::unique_ptr<Behavior> behavior) {
        behaviors.push_back(std::move(behavior));
    };
    void removeAllBehaviors() {
        behaviors.clear();
    }
    void executeBehavior(float deltaTime);

private:
    std::vector<std::unique_ptr<Behavior>> behaviors;
    bool markedForDeletion = false;
};

