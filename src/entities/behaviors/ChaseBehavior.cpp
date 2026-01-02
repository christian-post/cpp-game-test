#include "ChaseBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include <cmath>

ChaseBehavior::ChaseBehavior(Game& game, std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> targetSprite, float minDist)
    : game{ game }, self{ sprite }, other{ targetSprite }, minDist{ minDist }
{}

void ChaseBehavior::update(float deltaTime)
{
    if (auto s = self.lock(), o = other.lock(); s && o)
    {
        Vector2 selfCenter = GetRectCenter(s->rect);
        Vector2 otherCenter = GetRectCenter(o->rect);
        float dx = otherCenter.x - selfCenter.x;
        float dy = otherCenter.y - selfCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist <= minDist)
        {
            s->acc = { 0.0f, 0.0f };
            s->vel = { 0.0f, 0.0f };
        }
        else
        {
            s->acc.x = dx / dist;
            s->acc.y = dy / dist;
        }
    }
    // separation behavior between enemies
    for (auto& sprite : game.sprites)
    {
        if (!sprite->isEnemy)
            continue;

        Vector2 sum = { 0, 0 };
        int count = 0;
        float desiredSeparation = sprite->rect.width / 2.0f;

        for (auto& other : game.sprites)
        {
            if (other == sprite || !other->isEnemy)
                continue;

            float dx = sprite->position.x - other->position.x;
            float dy = sprite->position.y - other->position.y;
            float distSq = dx * dx + dy * dy;

            if (distSq >= desiredSeparation * desiredSeparation)
                continue;

            float dist = std::sqrt(distSq);
            Vector2 diff = { dx / dist, dy / dist };
            float mag = 1.0f / dist;
            diff.x *= mag;
            diff.y *= mag;
            sum.x += diff.x;
            sum.y += diff.y;
            count++;
        }

        if (count == 0)
            continue;

        sum.x /= count;
        sum.y /= count;
        float mag = std::sqrt(sum.x * sum.x + sum.y * sum.y);
        sum.x = sum.x / mag;
        sum.y = sum.y / mag;
        sprite->acc.x += (sum.x - sprite->acc.x);
        sprite->acc.y += (sum.y - sprite->acc.y);
    }
}
