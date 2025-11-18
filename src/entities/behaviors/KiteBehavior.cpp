#include "KiteBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include <cmath>

KiteBehavior::KiteBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, float orbitDistance, float moveSpeed)
    : game{ game }, self{ self }, target{ target }, orbitDistance{ orbitDistance }, moveSpeed{ moveSpeed }, orbitAngle{ 0.0f } {
    orbitAngle = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
}

void KiteBehavior::update(float deltaTime) {
    if (auto s = self.lock(), t = target.lock(); s && t) {
        Vector2 targetCenter = GetRectCenter(t->rect);

        orbitAngle += moveSpeed * deltaTime;
        if (orbitAngle > 2.0f * PI)
            orbitAngle -= 2.0f * PI;

        desiredPos.x = targetCenter.x + std::cos(orbitAngle) * orbitDistance;
        desiredPos.y = targetCenter.y + std::sin(orbitAngle) * orbitDistance;

        Vector2 selfCenter = GetRectCenter(s->rect);
        float dx = desiredPos.x - selfCenter.x;
        float dy = desiredPos.y - selfCenter.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 2.0f) {
            s->acc.x = dx / dist;
            s->acc.y = dy / dist;
        }
        else {
            s->acc = { 0.0f, 0.0f };
        }

        // separation behavior between enemies 
        // TODO this is duplicate code from ChaseBehavior, make a function?
        for (auto& sprite : game.sprites) {
            if (!sprite->isEnemy)
                continue;

            Vector2 sum = { 0, 0 };
            int count = 0;
            float desiredSeparation = sprite->rect.width / 2.0f;

            for (auto& other : game.sprites) {
                if (other != sprite && other->isEnemy) {
                    float dx = sprite->position.x - other->position.x;
                    float dy = sprite->position.y - other->position.y;
                    float distSq = dx * dx + dy * dy;
                    if (distSq < desiredSeparation * desiredSeparation) {
                        float dist = std::sqrt(distSq);
                        Vector2 diff = { dx / dist, dy / dist };
                        float mag = 1.0f / dist;
                        diff.x *= mag;
                        diff.y *= mag;
                        sum.x += diff.x;
                        sum.y += diff.y;
                        count++;
                    }
                }
            }
            if (count > 0) {
                sum.x /= count;
                sum.y /= count;

                float mag = std::sqrt(sum.x * sum.x + sum.y * sum.y);
                sum.x = sum.x / mag;
                sum.y = sum.y / mag;

                sprite->acc.x += (sum.x - sprite->acc.x);
                sprite->acc.y += (sum.y - sprite->acc.y);
            }
        }

        if (desiredPos.x < selfCenter.x)
            s->lastDirection = LEFT;
        else
            s->lastDirection = RIGHT;
    }
}

void KiteBehavior::draw()
{
    if (game.debug) {
        DrawCircle(static_cast<int>(desiredPos.x), static_cast<int>(desiredPos.y), 2.0f, RED);
        if (auto s = self.lock(); s)
            DrawLineEx(s->position, desiredPos, 1.0f, RED);
    }
}
