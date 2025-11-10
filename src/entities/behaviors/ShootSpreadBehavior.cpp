#include "ShootSpreadBehavior.h"
#include "ProjectileBehavior.h"
#include "EmitterBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include "Emitter.h"
#include "Particle.h"
#include <cmath>

ShootSpreadBehavior::ShootSpreadBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config, uint32_t projectileCount, float spreadAngle)
    : game{ game }, self{ self }, target{ target }, config{ config }, projectileCount{ projectileCount }, spreadAngle{ spreadAngle }, hasFired{ false } {
}

void ShootSpreadBehavior::update(float deltaTime) {
    if (hasFired) {
        done = true;
        return;
    }

    if (auto s = self.lock(), t = target.lock(); s && t) {
        game.playSound(config.sound);
        Vector2 sCenter = GetRectCenter(s->rect);
        Vector2 tCenter = GetRectCenter(t->rect);

        float baseAngle = std::atan2(tCenter.y - sCenter.y, tCenter.x - sCenter.x);
        float startAngle = baseAngle - spreadAngle / 2.0f;
        float angleStep = (projectileCount > 1) ? spreadAngle / (projectileCount - 1) : 0.0f;

        const auto& particlesData = game.loader.getParticleData();

        if (particlesData.find("defaultEmitter") == particlesData.end() || particlesData.find("defaultParticle") == particlesData.end()) {
            TraceLog(LOG_ERROR, "Missing 'defaultEmitter' or 'defaultParticle' in particles.json");
            hasFired = true;
            return;
        }

        const auto& defaultEmitterData = particlesData.at("defaultEmitter");
        const auto& defaultParticleData = particlesData.at("defaultParticle");

        for (uint32_t i = 0; i < projectileCount; i++) {
            float currentAngle = startAngle + angleStep * i;

            Rectangle sRect = { sCenter.x - config.hitboxSize / 2.0f, sCenter.y - config.hitboxSize / 2.0f, config.hitboxSize, config.hitboxSize };
            auto projectile = game.createSprite(config.projectileKey, sRect);
            projectile->setTextures({ config.projectileKey, config.projectileKey });
            projectile->canHurtPlayer = true;
            projectile->damage = config.damage;
            projectile->speed = config.speed;
            projectile->frameTime = config.frameTime;
            projectile->isColliding = false;

            Vector2 direction = { std::cos(currentAngle), std::sin(currentAngle) };
            projectile->acc = direction;
            projectile->addBehavior(std::make_unique<ProjectileBehavior>(game, projectile, t, false, direction));

            std::unique_ptr<Emitter> emitter;
            std::unique_ptr<Particle> proto = std::make_unique<Particle>();

            if (config.emitterKey.empty()) {
                emitter = std::make_unique<Emitter>(defaultEmitterData.value("maxParticles", 20));
                emitter->fromJSON(defaultEmitterData, defaultEmitterData);
                proto->fromJSON(defaultParticleData, defaultParticleData);
                proto->setAnimationFrames(game.loader.getTextures(config.projectileKey));
            }
            else {
                if (particlesData.find(config.emitterKey) == particlesData.end()) {
                    TraceLog(LOG_WARNING, "Emitter key \"%s\" not found in particles.json, using default", config.emitterKey.c_str());
                    emitter = std::make_unique<Emitter>(defaultEmitterData.value("maxParticles", 20));
                    emitter->fromJSON(defaultEmitterData, defaultEmitterData);
                    proto->fromJSON(defaultParticleData, defaultParticleData);
                    proto->setAnimationFrames(game.loader.getTextures(config.projectileKey));
                }
                else {
                    const auto& emitterData = particlesData.at(config.emitterKey);
                    std::string particleKey = emitterData.value("particleKey", defaultEmitterData.value("particleKey", "defaultParticle"));

                    if (particlesData.find(particleKey) == particlesData.end()) {
                        TraceLog(LOG_WARNING, "Particle key \"%s\" not found in particles.json", particleKey.c_str());
                        continue;
                    }

                    const auto& particleData = particlesData.at(particleKey);
                    size_t maxParticles = emitterData.value("maxParticles", defaultEmitterData.value("maxParticles", 20));

                    emitter = std::make_unique<Emitter>(maxParticles);
                    emitter->fromJSON(emitterData, defaultEmitterData);
                    proto->fromJSON(particleData, defaultParticleData);

                    std::string textureKey = particleData.value("textureKey", defaultParticleData.value("textureKey", "sprite_default"));
                    proto->setAnimationFrames(game.loader.getTextures(textureKey));
                }
            }

            emitter->location = sCenter;
            emitter->prototype = *proto;
            projectile->addBehavior(std::make_unique<EmitterBehavior>(game, projectile, std::move(emitter)));
        }

        hasFired = true;
    }
}

void ShootSpreadBehavior::reset() {
    Behavior::reset();
    hasFired = false;
}
