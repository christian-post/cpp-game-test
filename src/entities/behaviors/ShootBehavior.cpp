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
    : game{ game }, self{ self }, target{ target }, config{ config }
{
    interval = config.shootInterval;
}

void ShootBehavior::update(float deltaTime)
{
    timer += deltaTime;
    if (timer < interval)
        return;

    timer = 0.0f;
    if (auto s = self.lock(), t = target.lock(); s && t)
    {
        game.playSound(config.sound);
        Vector2 sCenter = GetRectCenter(s->rect);
        Rectangle sRect = { sCenter.x - config.hitboxSize / 2.0f, sCenter.y - config.hitboxSize / 2.0f, config.hitboxSize, config.hitboxSize };
        auto projectile = game.createSprite(config.projectileKey, sRect);
        projectile->setTextures({ config.projectileKey, config.projectileKey });
        // TODO trail and impact effect
        projectile->addBehavior(std::make_unique<ProjectileBehavior>(game, projectile, t, false, std::nullopt, config.projectileTrailEmitterKey));
        projectile->canHurtPlayer = true;
        projectile->damage = config.damage;
        projectile->speed = config.speed;
        projectile->frameTime = config.frameTime;
        projectile->emitsLight = true; // TODO is this always true?
    }
}

void ShootBehavior::reset()
{
    done = false;
    timer = std::numeric_limits<float>::infinity(); // immediately shoot the first shot
}
