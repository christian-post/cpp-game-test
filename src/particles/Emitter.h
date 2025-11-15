#pragma once
#include <vector>
#include <random>
#include <memory>
#include "raylib.h"
#include "raymath.h"
#include "Particle.h"
#include "json.hpp"


struct Emitter {
    // emits a certain type of Particle
    Emitter(size_t maxParticles);
    void emit();
    void update(float deltaTime);
    void draw();
    void reset();
    void fromJSON(const nlohmann::json& data, const nlohmann::json& defaultData);
    bool isDone() const;
    void start() { active = true; }
    void stop() { active = false; }

    bool active = true;  // flag to control emission
    Vector2 location = { 0.0f, 0.0f };
    float spawnInterval = 1.0f;
    float timeSinceLastSpawn = 0.0f;
    float emitterLifetime = -1.0f;
    float age = 0.0f;
    float spawnRadius = 0.0f;
    float spawnRadiusVariance = 0.0f;
    Vector2 velocityVariance = { 0.0f, 0.0f };
    float lifetimeVariance = 0.0f;
    float alphaVariance = 0.0f;

    // Radial velocity mode (for inward/outward particle movement)
    bool radialVelocity = false;
    float speed = 100.0f;
    float speedVariance = 0.0f;

    std::vector<Particle> particles;
    size_t maxParticles;
    Particle prototype;
    std::mt19937 rng;
};

class Game;

std::unique_ptr<Emitter> createEmitter(Game& game, std::string key); // helper function that creates an emitter from a key