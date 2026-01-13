#include "WatchBehavior.h"
#include "Sprite.h"

WatchBehavior::WatchBehavior(std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> targetSprite)
    : self{ sprite }, target{ targetSprite }
{}

void WatchBehavior::update(float deltaTime)
{
    if (auto s = self.lock(), t = target.lock(); s && t)
    {
        if (s->isMarkedForDeletion() || t->isMarkedForDeletion())
            return;

        if (t->position.x < s->position.x)
            s->lastDirection = LEFT;
        else
            s->lastDirection = RIGHT;
    }
}
