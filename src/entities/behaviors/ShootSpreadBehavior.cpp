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
    : game{ game }, self{ self }, target{ target }, config{ config }, projectileCount{ projectileCount }, spreadAngle{ spreadAngle }, hasFired{ false }
{}

void ShootSpreadBehavior::update(float deltaTime)
{
    if (hasFired)
    {
        done = true;
        return;
    }

    if (auto s = self.lock(), t = target.lock(); s && t)
    {
        game.playSound(config.sound);
        Vector2 sCenter = GetRectCenter(s->rect);
        Vector2 tCenter = GetRectCenter(t->rect);

        float baseAngle = std::atan2(tCenter.y - sCenter.y, tCenter.x - sCenter.x);
        float startAngle = baseAngle - spreadAngle / 2.0f;
        float angleStep = (projectileCount > 1) ? spreadAngle / (projectileCount - 1) : 0.0f;

        for (uint32_t i = 0; i < projectileCount; i++)
        {
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
            projectile->addBehavior(std::make_unique<ProjectileBehavior>(game, projectile, t, false, direction, config.projectileTrailEmitterKey, config.projectileImpactEmitterKey));
        }

        hasFired = true;
    }
}

void ShootSpreadBehavior::reset()
{
    Behavior::reset();
    hasFired = false;
}
