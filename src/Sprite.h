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

enum AnimState {
    IDLE,
    RUN,
    HIT
};


class Sprite {
public:
    Game& game;
    std::vector<std::vector<Texture2D>> frames;
    std::string spriteName; // used for general identification
    //std::string textureKey; // used for finding the correct texture
    uint32_t tileMapID = 0;
    int currentFrame = 0;
    bool doesAnimate = true;
    int drawLayer = 0;
    bool visible = true;
    bool persistent = false; // controls whether the sprite survives between map changes
    bool emitsLight = false; // in dark rooms, if the sprite gets a light cone
    AnimState currentAnimState = IDLE;

    std::optional<ShaderState> activeShader = std::nullopt;
    direction lastDirection = RIGHT;
    Color tint = WHITE;
    float rotationAngle = 0.0f;
    float frameTime = 0.12f; // animation speed
    float elapsedtime = 0.0f;

    // physics
    Rectangle rect; // hitbox for collision
    Vector2 hitboxOffset = { 0.0f, 0.0f }; // hitbox origin can differ from position
    Rectangle hurtbox; // hurtbox for attacks
    Vector2 hurtboxOffset = { 0.0f, 0.0f };
    bool isColliding = true;
    float speed = 20.0f; // default movement speed
    float jumpForce = 300.0f; 
    Vector2 acc;
    Vector2 vel;
    float friction = 0.8f;
    Vector2 position; // position exists independently of rect to allow for subpixel accurate movement
    bool staticCollision = false; // behaves like a wall
    // Z axis to simulate jumping
    // TODO: use a Vector3 at some point (needs heavy refactoring though)
    float z = 0.0f;
    float vz = 0.0f;
    float az = 0.0f;
    void jump();

    // gameplay variables
    uint32_t health; // current health
    uint32_t maxHealth;
    void refillHealth() { health = maxHealth; }
    float iFrameTimer = 0.0f; // duration of invincibility (s)
    bool canHurtPlayer = false;
    bool followsPlayer = false;
    bool isEnemy = false;
    uint32_t damage = 0;
    float knockback = 10.0f;
    bool dying = false; // flag for the death animation
    
    Sprite(Game& game, float x, float y, float w, float h, const std::string& spriteName);
    ~Sprite();
    void setTextures(std::vector<std::string> keys);
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
    void drawBehavior(); // TODO: good or bad design?

private:
    std::vector<std::unique_ptr<Behavior>> behaviors;
    bool markedForDeletion = false;
};

