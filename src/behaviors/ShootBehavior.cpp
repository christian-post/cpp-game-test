#include "ShootBehavior.h"
#include "ProjectileBehavior.h"
#include "EmitterBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include "Emitter.h"
#include "Particle.h"
#include <limits>


ShootBehavior::ShootBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config)
    : game{ game }, self{ self }, target{ target }, config{ config } {
    interval = config.shootInterval;
}

void ShootBehavior::update(float deltaTime) {
    timer += deltaTime;
    if (timer >= interval) {
        timer = 0.0f;
        if (auto s = self.lock(), t = target.lock(); s && t) {
            game.playSound(config.sound);
            Vector2 sCenter = GetRectCenter(s->rect);
            Rectangle sRect = { sCenter.x - config.hitboxSize / 2.0f, sCenter.y - config.hitboxSize / 2.0f, config.hitboxSize, config.hitboxSize };
            auto projectile = game.createSprite(config.projectileKey, sRect);
            projectile->setTextures({ config.projectileKey, config.projectileKey });
            projectile->addBehavior(std::make_unique<ProjectileBehavior>(game, projectile, t, false));
            projectile->canHurtPlayer = true;
            projectile->damage = config.damage;
            projectile->speed = config.speed;
            projectile->frameTime = config.frameTime;

            // trail effect (otional)
            if (config.emitterKey.empty())
                return;

            const auto& particlesData = game.loader.getParticleData();

            if (particlesData.find("defaultEmitter") == particlesData.end() || particlesData.find("defaultParticle") == particlesData.end()) {
                TraceLog(LOG_ERROR, "Missing 'defaultEmitter' or 'defaultParticle' in particles.json");
                return;
            }

            const auto& defaultEmitterData = particlesData.at("defaultEmitter");
            const auto& defaultParticleData = particlesData.at("defaultParticle");

            std::unique_ptr<Emitter> emitter;
            std::unique_ptr<Particle> proto = std::make_unique<Particle>();

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
                    return;
                }

                const auto& particleData = particlesData.at(particleKey);
                size_t maxParticles = emitterData.value("maxParticles", defaultEmitterData.value("maxParticles", 20));

                emitter = std::make_unique<Emitter>(maxParticles);
                emitter->fromJSON(emitterData, defaultEmitterData);
                proto->fromJSON(particleData, defaultParticleData);

                std::string textureKey = particleData.value("textureKey", defaultParticleData.value("textureKey", "sprite_default"));
                proto->setAnimationFrames(game.loader.getTextures(textureKey));
            }

            emitter->location = GetRectCenter(s->rect);
            emitter->prototype = *proto;
            projectile->addBehavior(std::make_unique<EmitterBehavior>(game, projectile, std::move(emitter)));
        }
    }
}

void ShootBehavior::reset()
{
    done = false;
    timer = std::numeric_limits<float>::infinity(); // immediately shoot the first shot
}
