#include "BombBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Emitter.h"
#include "Utils.h"

BombBehavior::BombBehavior(Game& game, std::shared_ptr<Sprite> self, bombConfig config)
    : game{ game }, self{ self }, config{ config }
{
}

void BombBehavior::update(float deltaTime)
{
    auto s = self.lock();
    if (!s)
        return;

    if (!exploded)
    {
        timer += deltaTime;

        // flash faster as fuse runs out
        float progress = timer / config.fuseTime;
        float flashSpeed = config.flashSpeedMin + progress * config.flashSpeedMax;
        bool flashOn = std::fmod(timer * flashSpeed, 1.0f) > 0.5f;
        s->tint = flashOn ? RED : WHITE;

        if (timer < config.fuseTime)
            return;

        exploded = true;
        s->tint = WHITE;
        // make bomb invisible as long as it hasn't been deleted
        s->visible = false;
        s->castsShadow = false;
        s->staticCollision = false;

        // spawn particle effect
        if (!config.emitterKey.empty())
        {
            auto emitter = createEmitter(game, config.emitterKey, {});
            emitter->position = GetRectCenter(s->rect);
            emitter->explode();
            game.emitters.emplace_back(std::move(emitter));
        }

        // spawn invisible explosion hitbox
        float r = config.explosionRadius;
        Rectangle explosionRect = CenterOnRect({ 0.0f, 0.0f, r, r }, s->rect);

        auto explosion = game.createSprite("explosion", explosionRect);
        explosion->visible = false;
        explosion->canHurtPlayer = false;
        explosion->canHurtEnemies = true;
        explosion->damage = config.damage;
        explosion->isColliding = false;
        explosion->damageType = DAMAGE_BOMB; // modify damage type
        explosion->speed = 0.0f; // hurtbox stays in place
        explosionSprite = explosion;
    }
    else
    {
        explosionTimer += deltaTime;
        if (explosionTimer >= config.explosionDuration)
        {
            if (auto explosion = explosionSprite.lock())
                explosion->markForDeletion();
            s->markForDeletion();
            done = true;
        }
    }
}