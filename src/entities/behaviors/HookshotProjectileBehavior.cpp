#include "HookshotProjectileBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include <cmath>

HookshotProjectileBehavior::HookshotProjectileBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> owner, Vector2 direction, float maxRange)
    : game{ game }, self{ self }, owner{ owner }, direction{ direction }, maxRange{ maxRange }
{
    if (auto s = this->self.lock())
        startPosition = GetRectCenter(s->rect);

    game.eventManager.addListener(HOOKSHOT_RETRACT, [this](std::any)
        {
            retracting = true;
        }, true);
}

void HookshotProjectileBehavior::update(float deltaTime)
{
    auto s = self.lock();
    if (!s || s->isMarkedForDeletion())
        return;

    if (retracting)
    {
        // only retracts when the projectile missed
        auto o = owner.lock();
        if (!o)
        {
            game.eventManager.pushEvent(HOOKSHOT_MISSED);
            s->markForDeletion();
            return;
        }

        Vector2 sCenter = GetRectCenter(s->rect);
        Vector2 oCenter = GetRectCenter(o->rect);

        float dist;
        if (direction.x != 0.0f)
            dist = std::abs(oCenter.x - sCenter.x);
        else
            dist = std::abs(oCenter.y - sCenter.y);

        if (dist < 16.0f) // TODO replace magic number
        {
            game.eventManager.pushEvent(HOOKSHOT_MISSED);
            s->markForDeletion();
            return;
        }

        float moveAmount = hookSpeed * deltaTime;
        s->position.x += -direction.x * moveAmount;
        s->position.y += -direction.y * moveAmount;
        s->rect.x = s->position.x;
        s->rect.y = s->position.y;
        return;
    }

    if (latched)
        return;

    float moveAmount = hookSpeed * deltaTime;
    s->position.x += direction.x * moveAmount;
    s->position.y += direction.y * moveAmount;
    s->rect.x = s->position.x;
    s->rect.y = s->position.y;

    if (direction.x != 0.0f)
        distanceTraveled = std::abs(s->position.x - startPosition.x);
    else
        distanceTraveled = std::abs(s->position.y - startPosition.y);

    if (distanceTraveled >= maxRange || !game.isInWorldBounds(s->rect))
    {
        retracting = true;
        return;
    }

    for (const auto& wall : game.walls)
    {
        if (wall->layer != 0) 
            continue; // excludes walls on other layers

        if (CheckCollisionRecs(s->rect, wall->getRect()))
        {
            // TODO particle effect
            game.eventManager.pushEvent(HOOKSHOT_RETRACT);
            return;
        }
    }

    for (const auto& target : game.sprites)
    {
        if (target.get() == s.get() || target->isMarkedForDeletion())
            continue;
        if (!CheckCollisionRecs(s->rect, target->rect))
            continue;

        if (target->hookshottable)
        {
            latchAt(GetRectCenter(target->rect));
            return;
        }
        if (target->isEnemy)
        {
            latchAt(GetRectCenter(target->rect), target);
            return;
        }
    }
}

void HookshotProjectileBehavior::latchAt(Vector2 position, std::weak_ptr<Sprite> enemy)
{
    auto s = self.lock();
    if (s)
    {
        s->acc = { 0.0f, 0.0f };
        s->vel = { 0.0f, 0.0f };
    }
    latched = true;

    HookshotLatchData latchData;
    latchData.latchPosition = position;
    latchData.latchedEnemy = enemy;
    game.eventManager.pushEvent(HOOKSHOT_LATCHED, std::make_any<HookshotLatchData>(latchData));
}