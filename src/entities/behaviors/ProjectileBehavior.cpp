#include "ProjectileBehavior.h"
#include "EmitterBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include "Emitter.h"
#include "Particle.h"
#include <cmath>

ProjectileBehavior::ProjectileBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, bool steer, std::optional<Vector2> customDirection, std::string trailEmitterKey, std::string impactEmitterKey)
    : game{ game }, self{ self }, target{ target }, steer{ steer }, trailEmitterKey{ trailEmitterKey}, impactEmitterKey{ impactEmitterKey }
{
    if (customDirection.has_value()) {
        direction = customDirection.value();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.0f) {
            direction.x /= length;
            direction.y /= length;
        }
    }
    else if (!steer && self && target) {
        Vector2 selfCenter = GetRectCenter(self->rect);
        Vector2 targetCenter = GetRectCenter(target->rect);
        float dx = targetCenter.x - selfCenter.x;
        float dy = targetCenter.y - selfCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        direction = { dx / dist, dy / dist };
    }
    self->isColliding = false;

    if (trailEmitterKey.length() > 0) {
        // create a trail effect
        std::unique_ptr<Emitter> emitter = createEmitter(game, trailEmitterKey);
        emitter->location = GetRectCenter(self->rect);
        self->addBehavior(std::make_unique<EmitterBehavior>(game, self, std::move(emitter)));
    }
}

void ProjectileBehavior::update(float deltaTime) {
    auto s = self.lock();
    auto t = target.lock();
    if (!s)
        return;

    if (impactEmitter) {
        if (impactEmitter->isDone()) {
            done = true;
            s->markForDeletion();
        }
        return;
    }

    for (const auto& wall : game.walls) {
        if (wall->layer == 0 && CheckCollisionRecs(s->rect, wall->getRect())) {
            createImpactEffect(s);
            return;
        }
    }

    if (t && (CheckCollisionRecs(s->rect, t->rect))) {
        createImpactEffect(s);
        return;
    }

    if (steer and t) {
        Vector2 selfCenter = GetRectCenter(s->rect);
        Vector2 targetCenter = GetRectCenter(t->rect);
        float dx = targetCenter.x - selfCenter.x;
        float dy = targetCenter.y - selfCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        s->acc.x = dx / dist;
        s->acc.y = dy / dist;
    }
    else {
        s->acc = direction;
    }
}

void ProjectileBehavior::draw() {
    if (impactEmitter)
        impactEmitter->draw();
}

// configures the emitter that spreads particles on impact
void ProjectileBehavior::createImpactEffect(std::shared_ptr<Sprite> s) {
    const auto& particlesData = game.loader.getParticleData();

    // TODO let the projectile define different emitters
    if (particlesData.find(impactEmitterKey) == particlesData.end()) {
        TraceLog(LOG_WARNING, "Impact Emitter '%s' not found in particles.json", impactEmitterKey.c_str());
        stopProjectile(s);
        return;
    }

    const auto& defaultEmitterData = particlesData.at("defaultEmitter");
    const auto& defaultParticleData = particlesData.at("defaultParticle");
    const auto& emitterData = particlesData.at(impactEmitterKey);

    size_t maxParticles = emitterData.value("maxParticles", defaultEmitterData.value("maxParticles", 20));
    auto emitter = std::make_unique<Emitter>(maxParticles);
    emitter->location = GetRectCenter(s->rect);
    emitter->fromJSON(emitterData, defaultEmitterData);

    std::string particleKey = emitterData.value("particleKey", defaultEmitterData.value("particleKey", "defaultParticle"));
    const auto& particleData = particlesData.at(particleKey);
    Particle proto;
    proto.fromJSON(particleData, defaultParticleData);
    proto.setAnimationFrames(s->frames[s->currentAnimState]);

    emitter->prototype = proto;

    impactEmitter = emitter.get();
    lifetimeAfterImpact = impactEmitter->emitterLifetime;
    game.emitters.push_back(std::move(emitter));

    stopProjectile(s);
}

void ProjectileBehavior::stopProjectile(std::shared_ptr<Sprite> s) {
    s->visible = false;
    s->vel = { 0.0f, 0.0f };
    s->acc = { 0.0f, 0.0f };
    s->canHurtPlayer = false;
    s->emitsLight = false;
    done = true;

    game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [s]() {
        s->removeAllBehaviors();
        });

    game.eventManager.pushDelayedEvent(UNNAMED, lifetimeAfterImpact, nullptr, [s]() {
        s->markForDeletion();
        });
}