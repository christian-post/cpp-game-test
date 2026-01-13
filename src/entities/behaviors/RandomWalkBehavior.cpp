#include "RandomWalkBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include <cmath>

RandomWalkBehavior::RandomWalkBehavior(std::shared_ptr<Sprite> sprite)
    : self{ sprite }
{
    if (auto s = self.lock())
        walkTarget = s->position;
    hasWalkTarget = false;
}

void RandomWalkBehavior::update(float deltaTime)
{
    if (auto s = self.lock())
    {
        if (s->isMarkedForDeletion())
            return;

        timer += deltaTime;

        if (timer < waitTime)
            return;

        float dx = walkTarget.x - s->position.x;
        float dy = walkTarget.y - s->position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq > 2.0f * 2.0f)
        {
            float dist = sqrtf(distSq);
            s->acc.x = dx / dist;
            s->acc.y = dy / dist;
        }
        else
        {
            s->acc = { 0.0f, 0.0f };
            s->vel = { 0.0f, 0.0f };
            hasWalkTarget = false;
        }

        if (hasWalkTarget)
            return;

        int tries = 20;
        while (tries-- > 0)
        {
            direction dir = static_cast<direction>(GetRandomValue(RIGHT, DOWN));
            int tiles = GetRandomValue(1, 4);
            float offset = tiles * 16.0f;
            Vector2 candidate = s->position;
            switch (dir)
            {
            case UP:
                candidate.y -= offset;
                break;
            case LEFT:
                candidate.x -= offset;
                break;
            case DOWN:
                candidate.y += offset;
                break;
            case RIGHT:
                candidate.x += offset;
                break;
            }
            Rectangle testRect = s->rect;
            testRect.x = candidate.x;
            testRect.y = candidate.y;

            if (s->game.isInWorldBounds(testRect) && isPathClear(s->rect, candidate, s->game.walls, s->layer))
            {
                walkTarget = candidate;
                hasWalkTarget = true;
                waitTime = float(rand() % 5 + 1);
                timer = 0.0f;
                break;
            }
        }
    }
}

void RandomWalkBehavior::reset()
{
    Behavior::reset();
    waitTime = 0.0f;
    hasWalkTarget = false;
    if (auto s = self.lock())
        walkTarget = s->position;
}
