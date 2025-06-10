#include "Particle.h"
#include "Emitter.h"
#include <cmath>

Emitter::Emitter(size_t maxParticles)
    : maxParticles(maxParticles), rng(std::random_device{}()) {
    particles.resize(maxParticles);
}

void Emitter::update(float deltaTime) {
    age += deltaTime;
    if (emitterLifetime > 0 && age >= emitterLifetime) return;

    timeSinceLastSpawn += deltaTime;
    while (timeSinceLastSpawn >= spawnInterval) {
        emit();
        timeSinceLastSpawn -= spawnInterval;
    }

    for (auto& p : particles) {
        if (p.active) {
            p.update(deltaTime);
        }
    }
}

void Emitter::draw() {
    for (auto& p : particles) {
        if (p.active) {
            p.draw();
        }
    }
}

void Emitter::emit() {
    for (auto& p : particles) {
        if (!p.active) {
            p = prototype;

            std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
            std::uniform_real_distribution<float> radiusOffset(-spawnRadiusVariance, spawnRadiusVariance);
            std::uniform_real_distribution<float> vxOffset(-velocityVariance.x, velocityVariance.x);
            std::uniform_real_distribution<float> vyOffset(-velocityVariance.y, velocityVariance.y);
            std::uniform_real_distribution<float> lifetimeOffset(-lifetimeVariance, lifetimeVariance);
            std::uniform_real_distribution<float> alphaOffset(-alphaVariance, alphaVariance);

            float angle = angleDist(rng);
            float radius = spawnRadius + radiusOffset(rng);
            Vector2 offset = { std::cos(angle) * radius, std::sin(angle) * radius };
            p.position = Vector2Add(location, offset);

            p.velocity.x += vxOffset(rng);
            p.velocity.y += vyOffset(rng);
            p.lifetime += lifetimeOffset(rng);
            p.alpha += alphaOffset(rng);

            p.reset();
            return;
        }
    }
}

Particle::Particle()
{
    startAlpha = alpha;
}

void Particle::update(float deltaTime) {
    if (!active) return;

    age += deltaTime;
    if (age >= lifetime) {
        active = false;
        return;
    }

    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    alpha = startAlpha + (endAlpha - startAlpha) * (age / lifetime);

    if (!animationFrames.empty()) {
        animationTimer += deltaTime;
        if (animationTimer >= animationSpeed) {
            animationTimer = 0.0f;
            currentFrame = (currentFrame + 1) % animationFrames.size();
        }
    }
}

void Particle::draw() {
    if (!active || animationFrames.empty()) return;

    Texture2D* tex = animationFrames[currentFrame];
    if (!tex) return;

    Color finalColor = tint;
    finalColor.a = static_cast<unsigned char>(alpha * 255.0f);

    Vector2 origin = { tex->width / 2.0f, tex->height / 2.0f };
    DrawTexturePro(*tex,
        { 0, 0, static_cast<float>(tex->width), static_cast<float>(tex->height) },
        { position.x, position.y, static_cast<float>(tex->width), static_cast<float>(tex->height) },
        origin,
        0.0f,
        finalColor);
}

void Particle::reset() {
    age = 0.0f;
    currentFrame = 0;
    animationTimer = 0.0f;
    startAlpha = alpha;
    active = true;
}

void Particle::setAnimationFrames(const std::vector<Texture2D>& textures) {
    animationFrames.clear();
    for (const auto& tex : textures) {
        animationFrames.push_back(const_cast<Texture2D*>(&tex));
    }
}
