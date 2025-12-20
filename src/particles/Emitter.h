#pragma once
#include <vector>
#include <random>
#include <memory>
#include "raylib.h"
#include "raymath.h"
#include "Particle.h"
#include "json.hpp"

enum class EmitterState {
    Stopped,      // not emitting
    Continuous,   // infinite emission
    Timed,        // finite lifetime emission
    Burst         // one-shot explosion with auto-cleanup
};

struct Emitter {
    // emits a certain type of Particle
    Emitter(size_t maxParticles);
    void emit();
    void update(float deltaTime);
    void draw();
    void reset();
    void fromJSON(const nlohmann::json& data, const nlohmann::json& defaultData, const nlohmann::json& overrideData = nlohmann::json::object());
    bool isDone() const;
    void explode(); // emit once, then stop
    void start();
    void stop();

    EmitterState state = EmitterState::Continuous;
    bool persistent = false; // whether this survives room changes
    Vector2 position = { 0.0f, 0.0f };
    size_t burstSize = 1;
    float spawnInterval = 1.0f;
    float spawnDelay = 0.0f;
    float timer = 0.0f;
    float emitterLifetime = -1.0f;
    float age = 0.0f;
    float spawnRadius = 0.0f;
    float spawnRadiusVariance = 0.0f;
    float lifetimeVariance = 0.0f;
    float alphaVariance = 0.0f;
    float startSizeVariance = 0.0f;
    float endSizeVariance = 0.0f;
    Color tint = WHITE; // overwrites particle tint
    std::vector<Vector3> colorGradient; // overwrites Particle colors (for gradients)
    Vector2 gravity = { 0.0f, 0.0f }; // modifies velocity
    EasingType velocityEasing = EasingType::None; // can override easing of the particle velocity

    // Radial velocity (all particles use this)
    float speed = 100.0f;
    float speedVariance = 0.0f;

    std::vector<Particle> particles;
    size_t maxParticles;
    Particle prototype;
    Particle createParticle(Particle p);
    std::mt19937 rng;
};

class Game;

std::shared_ptr<Emitter> createEmitter(Game& game, std::string key, const nlohmann::json& overrideData = nlohmann::json::object()); // helper function that creates an emitter from a key (optional json data for overrides)