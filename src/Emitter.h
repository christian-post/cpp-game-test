#pragma once

#include <vector>
#include <random>
#include "raymath.h"
#include "Particle.h"

struct Emitter {
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

    std::vector<Particle> particles;
    size_t maxParticles;

    Particle prototype;

    std::mt19937 rng;

    Emitter(size_t maxParticles);
    void emit();
    void update(float deltaTime);
    void draw();
};
