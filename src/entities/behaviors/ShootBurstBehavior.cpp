#include "ShootBurstBehavior.h"
#include "ProjectileBehavior.h"
#include "EmitterBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include "Emitter.h"
#include "Particle.h"

ShootBurstBehavior::ShootBurstBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config, uint32_t burstCount, float burstDelay)
    : game{ game }, self{ self }, target{ target }, config{ config }, burstCount{ burstCount }, shotsFired{ 0 }, burstDelay{ burstDelay }
{}

void ShootBurstBehavior::update(float deltaTime)
{
    if (shotsFired >= burstCount)
    {
        done = true;
        return;
    }

    timer += deltaTime;
    if (timer < burstDelay)
        return;

    timer = 0.0f;

    if (auto s = self.lock(), t = target.lock(); s && t)
    {
        if (s->isMarkedForDeletion() || t->isMarkedForDeletion())
            return;

        game.playSound(config.sound);
        Vector2 sCenter = GetRectCenter(s->rect);
        Rectangle sRect = { sCenter.x - config.hitboxSize / 2.0f, sCenter.y - config.hitboxSize / 2.0f, config.hitboxSize, config.hitboxSize };
        auto projectile = game.createSprite(config.projectileKey, sRect);
        projectile->setTextures({ config.projectileKey, config.projectileKey });
        projectile->addBehavior(std::make_unique<ProjectileBehavior>(game, projectile, t, false, std::nullopt, config.projectileTrailEmitterKey, config.projectileImpactEmitterKey));
        projectile->canHurtPlayer = true;
        projectile->damage = config.damage;
        projectile->speed = config.speed;
        projectile->frameTime = config.frameTime;

        shotsFired++;
    }
}

void ShootBurstBehavior::reset()
{
    Behavior::reset();
    shotsFired = 0;
}
