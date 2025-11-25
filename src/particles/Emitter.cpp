#include "Emitter.h"
#include "Game.h"
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
    startSizeVariance = data.value("startSizeVariance", defaultData.value("startSizeVariance", 0.0f));
    endSizeVariance = data.value("endSizeVariance", defaultData.value("endSizeVariance", 0.0f));

    spawnDelay = data.value("spawnDelay", defaultData.value("spawnDelay", 0.0f));
    timer = spawnInterval - spawnDelay;

    if (data.contains("tint")) {
        auto& tintArray = data.at("tint");
        tint = Color{
            static_cast<unsigned char>(tintArray[0].get<int>()),
            static_cast<unsigned char>(tintArray[1].get<int>()),
            static_cast<unsigned char>(tintArray[2].get<int>()),
            static_cast<unsigned char>(tintArray[3].get<int>())
        };
    }
}

bool Emitter::isDone() const {
    // Infinite emitters (lifetime == -1) are never "done" on their own
    if (emitterLifetime <= 0)
        return false;

    // If emitter has a positive lifetime and hasn't finished spawning, not done
    if (age < emitterLifetime)
        return false;

    // Check if any particles are still active
    for (const auto& p : particles) {
        if (p.active)
            return false;
    }

    // Emitter finished spawning AND all particles are dead
    return true;
}

Particle Emitter::createParticle(Particle p)
{
    p = prototype;

    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * PI);
    std::uniform_real_distribution<float> radiusOffset(-spawnRadiusVariance, spawnRadiusVariance);
    std::uniform_real_distribution<float> lifetimeOffset(-lifetimeVariance, lifetimeVariance);
    std::uniform_real_distribution<float> alphaOffset(-alphaVariance, alphaVariance);
    std::uniform_real_distribution<float> startSizeOffset(-startSizeVariance, startSizeVariance);
    std::uniform_real_distribution<float> endSizeOffset(-endSizeVariance, endSizeVariance);

    float angle = angleDist(rng);
    float radius = spawnRadius + radiusOffset(rng);
    Vector2 offset = { std::cos(angle) * radius, std::sin(angle) * radius };
    p.position = Vector2Add(position, offset);

    if (radialVelocity) {
        // Calculate direction from spawn position to center (inward)
        Vector2 direction = Vector2Subtract(position, p.position);
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);

        if (length > 0.0f) {
            direction.x /= length;
            direction.y /= length;
        }

        std::uniform_real_distribution<float> speedOffset(-speedVariance, speedVariance);
        float finalSpeed = speed + speedOffset(rng);

        p.velocity.x += direction.x * finalSpeed;
        p.velocity.y += direction.y * finalSpeed;
    }
    else {
        std::uniform_real_distribution<float> vxOffset(-velocityVariance.x, velocityVariance.x);
        std::uniform_real_distribution<float> vyOffset(-velocityVariance.y, velocityVariance.y);
        p.velocity.x += vxOffset(rng);
        p.velocity.y += vyOffset(rng);
    }

    p.lifetime += lifetimeOffset(rng);
    p.alpha += alphaOffset(rng);
    p.startSize += startSizeOffset(rng);
    p.endSize += startSizeOffset(rng);

    p.tint = tint;

    p.reset();
    return p;
}

void Emitter::update(float deltaTime) {
    age += deltaTime;

    // Only emit new particles if within lifetime
    if (emitterLifetime <= 0 || age < emitterLifetime) {
        if (active) {
            timer += deltaTime;
            if (timer >= spawnInterval) {
                emit();
                timer = 0.0f;
            }
        }
    }

    // Always update existing particles regardless of emitter lifetime
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
    timer = spawnInterval - spawnDelay;
}

void Emitter::emit() {
    for (auto& p : particles) {
        if (!p.active) {
            p = createParticle(p);
        }
    }
}


std::unique_ptr<Emitter> createEmitter(Game& game, std::string key)
{
    const auto& particlesData = game.loader.getParticleData();
    const auto& emitterData = particlesData.at(key);
    std::string particleKey = emitterData.at("particleKey");
    const auto& particleData = particlesData.at(particleKey);
    size_t maxParticles = emitterData.at("maxParticles");

    std::unique_ptr<Emitter> emitter = std::make_unique<Emitter>(maxParticles);
    std::unique_ptr<Particle> proto = std::make_unique<Particle>();

    emitter->fromJSON(emitterData, particlesData.at("defaultEmitter"));
    proto->fromJSON(particleData, particlesData.at("defaultParticle"));

    std::string textureKey = particleData.at("textureKey");
    proto->setAnimationFrames(game.loader.getTextures(textureKey));
    emitter->prototype = *proto;

    return emitter;
}
