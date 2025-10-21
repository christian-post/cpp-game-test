#include "Emitter.h"
#include <cmath>


Emitter::Emitter(size_t maxParticles)
    : maxParticles(maxParticles), rng(std::random_device{}()) {
    particles.resize(maxParticles);
}

void Emitter::fromJSON(const nlohmann::json& data, const nlohmann::json& defaultData) {
    spawnInterval = data.value("spawnInterval", defaultData.value("spawnInterval", 1.0f));
    emitterLifetime = data.value("emitterLifetime", defaultData.value("emitterLifetime", -1.0f));
    spawnRadius = data.value("spawnRadius", defaultData.value("spawnRadius", 0.0f));
    spawnRadiusVariance = data.value("spawnRadiusVariance", defaultData.value("spawnRadiusVariance", 0.0f));
    velocityVariance.x = data.value("velocityVarianceX", defaultData.value("velocityVarianceX", 0.0f));
    velocityVariance.y = data.value("velocityVarianceY", defaultData.value("velocityVarianceY", 0.0f));
    lifetimeVariance = data.value("lifetimeVariance", defaultData.value("lifetimeVariance", 0.0f));
    alphaVariance = data.value("alphaVariance", defaultData.value("alphaVariance", 0.0f));
    radialVelocity = data.value("radialVelocity", defaultData.value("radialVelocity", false));
    speed = data.value("speed", defaultData.value("speed", 1.0f));
    speedVariance = data.value("speedVariance", defaultData.value("speedVariance", 0.0f));
}

bool Emitter::isDone() const {
    // If emitter has a positive lifetime and hasn't finished spawning, not done
    if (emitterLifetime > 0 && age < emitterLifetime)
        return false;

    // Check if any particles are still active
    for (const auto& p : particles) {
        if (p.active)
            return false;
    }

    // Emitter finished spawning AND all particles are dead
    return true;
}

void Emitter::update(float deltaTime) {
    age += deltaTime;
    if (emitterLifetime > 0 && age >= emitterLifetime)
        return;

    timeSinceLastSpawn += deltaTime;
    while (timeSinceLastSpawn >= spawnInterval) {
        emit();
        timeSinceLastSpawn -= spawnInterval;
    }

    for (auto& p : particles) {
        if (p.active)
            p.update(deltaTime);
    }
}

void Emitter::draw() {
    for (auto& p : particles) {
        if (p.active)
            p.draw();
    }
}

void Emitter::reset() {
    for (auto& p : particles)
        p.active = false;
    age = 0.0f;
    timeSinceLastSpawn = 0.0f;
}

void Emitter::emit() {
    for (auto& p : particles) {
        if (!p.active) {
            p = prototype;

            std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
            std::uniform_real_distribution<float> radiusOffset(-spawnRadiusVariance, spawnRadiusVariance);
            std::uniform_real_distribution<float> lifetimeOffset(-lifetimeVariance, lifetimeVariance);
            std::uniform_real_distribution<float> alphaOffset(-alphaVariance, alphaVariance);

            float angle = angleDist(rng);
            float radius = spawnRadius + radiusOffset(rng);
            Vector2 offset = { std::cos(angle) * radius, std::sin(angle) * radius };
            p.position = Vector2Add(location, offset);

            if (radialVelocity) {
                // Calculate direction from spawn position to center (inward)
                Vector2 direction = Vector2Subtract(location, p.position);
                float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

                if (length > 0.0f) {
                    direction.x /= length;
                    direction.y /= length;
                }

                std::uniform_real_distribution<float> speedOffset(-speedVariance, speedVariance);
                float finalSpeed = speed + speedOffset(rng);

                p.velocity = { direction.x * finalSpeed, direction.y * finalSpeed };
            }
            else {
                std::uniform_real_distribution<float> vxOffset(-velocityVariance.x, velocityVariance.x);
                std::uniform_real_distribution<float> vyOffset(-velocityVariance.y, velocityVariance.y);
                p.velocity.x += vxOffset(rng);
                p.velocity.y += vyOffset(rng);
            }

            p.lifetime += lifetimeOffset(rng);
            p.alpha += alphaOffset(rng);

            p.reset();
            return;
        }
    }
}