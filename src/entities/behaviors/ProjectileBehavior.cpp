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
    if (customDirection.has_value())
    {
        direction = customDirection.value();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.0f)
        {
            direction.x /= length;
            direction.y /= length;
        }
    }
    else if (!steer && self && target)
    {
        Vector2 selfCenter = GetRectCenter(self->rect);
        Vector2 targetCenter = GetRectCenter(target->rect);
        float dx = targetCenter.x - selfCenter.x;
        float dy = targetCenter.y - selfCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        direction = { dx / dist, dy / dist };
    }
    self->isColliding = false;
    self->layer = 0; // TODO change this when the collision layer system is fixed

    if (trailEmitterKey.length() > 0)
    {
        // create a trail effect
        std::shared_ptr<Emitter> emitter = createEmitter(game, trailEmitterKey);
        emitter->position = GetRectCenter(self->rect);
        self->addBehavior(std::make_unique<EmitterBehavior>(game, self, std::move(emitter)));
    }
}

void ProjectileBehavior::update(float deltaTime)
{
    auto s = self.lock();
    auto t = target.lock();

    if (!s)
        return;

    if (s->isMarkedForDeletion() || (t && t->isMarkedForDeletion()))
        return;

    if (impactEmitter)
    {
        if (impactEmitter->isDone())
        {
            done = true;
            s->markForDeletion();
        }
        return;
    }

    for (const auto& wall : game.walls)
    {
        if (wall->layer == s->layer && CheckCollisionRecs(s->rect, wall->getRect()))
        {
            createImpactEffect(s);
            return;
        }
    }

    // TODO check every sprite that can be damaged
    if (t && (CheckCollisionRecs(s->rect, t->rect)))
    {
        createImpactEffect(s);
        return;
    }

    if (steer and t)
    {
        Vector2 selfCenter = GetRectCenter(s->rect);
        Vector2 targetCenter = GetRectCenter(t->rect);
        float dx = targetCenter.x - selfCenter.x;
        float dy = targetCenter.y - selfCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        s->acc.x = dx / dist;
        s->acc.y = dy / dist;
    }
    else
    {
        s->acc = direction;
    }
}

void ProjectileBehavior::draw()
{
    if (impactEmitter)
        impactEmitter->draw();
}

void ProjectileBehavior::createImpactEffect(std::shared_ptr<Sprite> s)
{
    if (impactEmitterKey.empty())
    {
        stopProjectile(s);
        return;
    }

    std::shared_ptr<Emitter> emitter = createEmitter(game, impactEmitterKey);
    if (!emitter)
    {
        TraceLog(LOG_WARNING, "Failed to create impact emitter '%s'", impactEmitterKey.c_str());
        stopProjectile(s);
        return;
    }

    emitter->position = GetRectCenter(s->rect);

    // Set particle animation frames from sprite
    emitter->prototype.setAnimationFrames(s->frames[s->currentAnimState]);

    impactEmitter = emitter.get();
    lifetimeAfterImpact = impactEmitter->emitterLifetime;
    game.emitters.push_back(emitter);

    stopProjectile(s);
}

void ProjectileBehavior::stopProjectile(std::shared_ptr<Sprite> s)
{
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