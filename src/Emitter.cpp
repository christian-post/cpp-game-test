#include "Emitter.h"
#include <cmath>


Emitter::Emitter(size_t maxParticles)
    : maxParticles(maxParticles), rng(std::random_device{}()) {
    particles.resize(maxParticles);
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