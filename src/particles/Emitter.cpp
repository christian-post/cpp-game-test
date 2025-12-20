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

    // Set initial state based on configuration
    if (emitterLifetime < 0)
        state = EmitterState::Continuous;
    else
        state = EmitterState::Timed;

    spawnRadius = getValue("spawnRadius", 0.0f);
    spawnRadiusVariance = getValue("spawnRadiusVariance", 0.0f);
    lifetimeVariance = getValue("lifetimeVariance", 0.0f);
    alphaVariance = getValue("alphaVariance", 0.0f);
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
    //const nlohmann::json* tintSource = nullptr;
    //if (overrideData.contains("tint"))
    //    tintSource = &overrideData;
    //else if (data.contains("tint"))
    //    tintSource = &data;

    //if (tintSource) {
    //    auto& tintArray = tintSource->at("tint");
    //    tint = Color{
    //        static_cast<unsigned char>(tintArray[0].get<int>()),
    //        static_cast<unsigned char>(tintArray[1].get<int>()),
    //        static_cast<unsigned char>(tintArray[2].get<int>()),
    //        static_cast<unsigned char>(tintArray[3].get<int>())
    //    };
    //}
    const nlohmann::json* tintSource = nullptr;
    if (overrideData.contains("tint"))
        tintSource = &overrideData;
    else if (data.contains("tint"))
        tintSource = &data;

    if (tintSource) {
        auto& tintData = tintSource->at("tint");
        if (tintData[0].is_array()) {
            // Multi-color gradient
            colorGradient.clear();
            for (const auto& colorArray : tintData) {
                Vector3 normalized = {
                    colorArray[0].get<int>() / 255.0f,
                    colorArray[1].get<int>() / 255.0f,
                    colorArray[2].get<int>() / 255.0f
                };
                colorGradient.push_back(normalized);
            }
        }
        else {
            // Single color - backward compatibility
            tint = Color{
                static_cast<unsigned char>(tintData[0].get<int>()),
                static_cast<unsigned char>(tintData[1].get<int>()),
                static_cast<unsigned char>(tintData[2].get<int>()),
                static_cast<unsigned char>(tintData[3].get<int>())
            };
            // Also store as gradient for consistency
            Vector3 normalized = {
                tintData[0].get<int>() / 255.0f,
                tintData[1].get<int>() / 255.0f,
                tintData[2].get<int>() / 255.0f
            };
            colorGradient = { normalized };
        }
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
    // Continuous and stopped emitters are never "done"
    if (state == EmitterState::Continuous || state == EmitterState::Stopped)
        return false;

    // Timed emitters aren't done until they finish their lifetime
    if (state == EmitterState::Timed && age < emitterLifetime)
        return false;

    // Burst emitters (and timed past lifetime) are done when all particles die
    for (const auto& p : particles) {
        if (p.active)
            return false;
    }

    return true;
}

void Emitter::start()
{
    if (state == EmitterState::Stopped) {
        if (emitterLifetime < 0)
            state = EmitterState::Continuous;
        else
            state = EmitterState::Timed;
    }
}

void Emitter::stop()
{
    state = EmitterState::Stopped;
}

void Emitter::explode()
{
    age = 0.0f;
    state = EmitterState::Burst;
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

    // All particles use radial velocity: direction from spawn angle + speed
    // Positive speed = outward, negative = inward, zero = stationary
    Vector2 direction = { std::cos(angle), std::sin(angle) };

    std::uniform_real_distribution<float> speedOffset(-speedVariance, speedVariance);
    float finalSpeed = speed + speedOffset(rng);

    p.velocity.x += direction.x * finalSpeed;
    p.velocity.y += direction.y * finalSpeed;

    p.lifetime += lifetimeOffset(rng);
    p.alpha += alphaOffset(rng);
    p.startSize += startSizeOffset(rng);
    p.endSize += startSizeOffset(rng);
    p.gravity = gravity;
    if (velocityEasing != EasingType::None) {
        p.velocityEasing = velocityEasing;
    }

    //p.tint = tint;
    if (!colorGradient.empty())
        p.colorGradient = colorGradient;

    p.reset();
    return p;
}

void Emitter::update(float deltaTime) {
    age += deltaTime;

    // Spawn new particles based on state
    if (state == EmitterState::Continuous || (state == EmitterState::Timed && age < emitterLifetime)) {
        timer += deltaTime;
        if (timer >= 0.0f) {
            emit();
            timer -= spawnInterval;
        }
    }

    // Update all active particles
    for (auto& p : particles) {
        if (p.active)
            p.update(deltaTime);
    }

    // Auto-cleanup for burst state
    if (state == EmitterState::Burst && isDone()) {
        reset();
        state = EmitterState::Stopped;
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