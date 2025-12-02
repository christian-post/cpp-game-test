#include "Emitter.h"
#include "Game.h"
#include <cmath>


Emitter::Emitter(size_t maxParticles)
    : maxParticles(maxParticles), rng(std::random_device{}()) {
    particles.resize(maxParticles);
}

void Emitter::fromJSON(const nlohmann::json& data, const nlohmann::json& defaultData, const nlohmann::json& overrideData) {
    // lambda func to get value with override priority
    auto getValue = [&](const char* key, auto defaultValue) {
        if (overrideData.contains(key))
            return overrideData.value(key, defaultValue);
        return data.value(key, defaultData.value(key, defaultValue));
        };

    spawnInterval = getValue("spawnInterval", 1.0f);
    burstSize = getValue("burstSize", size_t(1));
    emitterLifetime = getValue("emitterLifetime", -1.0f);
    spawnRadius = getValue("spawnRadius", 0.0f);
    spawnRadiusVariance = getValue("spawnRadiusVariance", 0.0f);
    velocityVariance.x = getValue("velocityVarianceX", 0.0f);
    velocityVariance.y = getValue("velocityVarianceY", 0.0f);
    lifetimeVariance = getValue("lifetimeVariance", 0.0f);
    alphaVariance = getValue("alphaVariance", 0.0f);
    radialVelocity = getValue("radialVelocity", false);
    speed = getValue("speed", 1.0f);
    speedVariance = getValue("speedVariance", 0.0f);
    startSizeVariance = getValue("startSizeVariance", 0.0f);
    endSizeVariance = getValue("endSizeVariance", 0.0f);
    spawnDelay = getValue("spawnDelay", 0.0f);

    // Handle gravity (check override first, then data)
    if (overrideData.contains("gravity")) {
        gravity.x = overrideData["gravity"]["x"];
        gravity.y = overrideData["gravity"]["y"];
    }
    else if (data.contains("gravity")) {
        gravity.x = data["gravity"]["x"];
        gravity.y = data["gravity"]["y"];
    }

    timer = -spawnDelay;

    // Handle tint (check override first, then data)
    const nlohmann::json* tintSource = nullptr;
    if (overrideData.contains("tint"))
        tintSource = &overrideData;
    else if (data.contains("tint"))
        tintSource = &data;

    if (tintSource) {
        auto& tintArray = tintSource->at("tint");
        tint = Color{
            static_cast<unsigned char>(tintArray[0].get<int>()),
            static_cast<unsigned char>(tintArray[1].get<int>()),
            static_cast<unsigned char>(tintArray[2].get<int>()),
            static_cast<unsigned char>(tintArray[3].get<int>())
        };
    }

    // velocityEasing
    const nlohmann::json* easingSource = nullptr;
    if (overrideData.contains("velocityEasing"))
        easingSource = &overrideData;
    else if (data.contains("velocityEasing"))
        easingSource = &data;

    if (easingSource) {
        std::string easingStr = easingSource->at("velocityEasing");
        if (easingStr == "quadIn")
            velocityEasing = EasingType::QuadIn;
        else if (easingStr == "quadOut")
            velocityEasing = EasingType::QuadOut;
        else if (easingStr == "quadInOut")
            velocityEasing = EasingType::QuadInOut;
        else if (easingStr == "cubicIn")
            velocityEasing = EasingType::CubicIn;
        else if (easingStr == "cubicOut")
            velocityEasing = EasingType::CubicOut;
        else if (easingStr == "cubicInOut")
            velocityEasing = EasingType::CubicInOut;
        else
            velocityEasing = EasingType::None;
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

void Emitter::explode()
{
    start();
    emit();
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
    p.gravity = gravity;
    if (velocityEasing != EasingType::None) {
        p.velocityEasing = velocityEasing;
    }

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
            if (timer >= 0.0f) {
                emit();
                timer -= spawnInterval;
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
    timer = -spawnDelay;
}

void Emitter::emit() {
    size_t spawned = 0;
    for (auto& p : particles) {
        if (!p.active) {
            p = createParticle(p);
            spawned++;
            if (spawned >= burstSize)
                return;
        }
    }
}


std::shared_ptr<Emitter> createEmitter(Game& game, std::string key, const nlohmann::json& overrideData)
{
    const auto& particlesData = game.loader.getParticleData();
    const auto& emitterData = particlesData.at(key);
    std::string particleKey = emitterData.at("particleKey");
    const auto& particleData = particlesData.at(particleKey);
    size_t maxParticles = emitterData.at("maxParticles");

    std::shared_ptr<Emitter> emitter = std::make_shared<Emitter>(maxParticles);
    std::unique_ptr<Particle> proto = std::make_unique<Particle>();

    emitter->fromJSON(emitterData, particlesData.at("defaultEmitter"), overrideData);
    proto->fromJSON(particleData, particlesData.at("defaultParticle"));

    if (particleData.contains("textureKey")) {
        std::string textureKey = particleData.at("textureKey");
        proto->setAnimationFrames(game.loader.getTextures(textureKey));
    }
    emitter->prototype = *proto;

    return emitter;
}
