#pragma once
#include "Behavior.h"
#include "StateMachine.h"
#include "raylib.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>

class Game;

struct ShaderState
{
    const Shader* shader = nullptr;
    float time = 0.0f;
    float duration = 2.0f;
    int flipX = 0;
};

enum AnimState
{
    IDLE,
    RUN,
    HIT,
    CHARGE,
    NUM_ANIM_STATES
};

enum DamageType : uint32_t
{
    DAMAGE_NONE = 0,
    DAMAGE_NORMAL = 1 << 0,
    DAMAGE_BOMB = 1 << 1,
    DAMAGE_FIRE = 1 << 2,
    DAMAGE_ICE = 1 << 3,
}; // types of damage that trigger certain behavior


class Sprite
{
public:
    Game& game;
    std::vector<std::vector<Texture2D>> frames; // outer index refers to currentAnimState; inner index refers to currentFrame
    std::string spriteName; // used for general identification
    AnimState currentAnimState = IDLE;
    size_t currentFrame = 0;
    uint32_t tileMapID = 0; // Unique identifier for Room creation (saving and loading the sprite data)
    bool doesAnimate = true;
    int drawLayer = 0;
    bool visible = true;
    bool persistent = false; // controls whether the sprite survives between map changes
    bool emitsLight = false; // in dark rooms, if the sprite gets a light cone
    bool castsShadow = true;

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
    float jumpForce = 600.0f; 
    Vector2 acc;
    Vector2 vel;
    float friction = 0.8f;
    Vector2 position; // position exists independently of rect to allow for subpixel accurate movement
    bool staticCollision = false; // behaves like a wall
    int layer = 0;  // collision layer
    // Z axis to simulate jumping
    // TODO: use a Vector3 at some point (needs heavy refactoring though)
    float z = 0.0f;
    float vz = 0.0f;
    float az = 0.0f;
    void jump(uint32_t force = 600);

    // gameplay variables
    uint32_t health; // current health
    uint32_t maxHealth;
    void refillHealth() { health = maxHealth; }
    float iFrameTimer = 0.0f; // duration of invincibility (s)
    bool canHurtPlayer = false;
    bool canHurtEnemies = false;
    bool followsPlayer = false;
    bool isEnemy = false;
    uint32_t damage = 0;
    uint32_t damageType = DAMAGE_NORMAL;
    uint32_t immunities = DAMAGE_NONE;
    float knockback = 10.0f; // knockback that the sprite inflicts to others
    int weight = 1; // influences the knockback that the sprite iselfs experiences
    bool dying = false; // flag for the death animation
    bool hookshottable = false; // can the hookshot's hook grab onto this?
    
    Sprite(Game& game, float x, float y, float w, float h, const std::string& spriteName);
    ~Sprite();
    void setTextures(std::vector<std::string> keys);
    void animate(float deltaTime);
    void setAnimState(AnimState state, bool lockState = false);
    void LockAnimState() { lockedAnimState = true; }
    void unlockAnimState() { lockedAnimState = false; }
    void setHurtbox(float x = -1.0f, float y = -1.0f, float width = -1.0f, float height = -1.0f, bool center = false);
    void getControls();
    void update(float deltaTime);
    void drawShadow();
    void draw();
    void moveTo(float x, float y);

    bool isMarkedForDeletion() const { return markedForDeletion; }
    void markForDeletion() { markedForDeletion = true; }

    // behavior methods
    void addBehavior(std::unique_ptr<Behavior> behavior)
    {
        behaviors.push_back(std::move(behavior));
    };
    void removeAllBehaviors()
    {
        for (auto& behavior : behaviors)
        {
            if (behavior)
                behavior->onDeactivate();
        }
        behaviors.clear();
    }
    void executeBehavior(float deltaTime);
    void drawBehavior();

    // State machine functions
    std::unique_ptr<StateMachine> stateMachine = nullptr;

    // Add method to set up state machine
    void setStateMachine(std::unique_ptr<StateMachine> sm)
    {
        stateMachine = std::move(sm);
    }

    bool hasStateMachine() const
    {
        return stateMachine != nullptr;
    }

private:
    std::vector<std::unique_ptr<Behavior>> behaviors;
    bool markedForDeletion = false;
    bool lockedAnimState = false;
};

