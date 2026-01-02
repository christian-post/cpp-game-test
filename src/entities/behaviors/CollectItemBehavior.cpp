#include "CollectItemBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include <any>
#include <cmath>

CollectItemBehavior::CollectItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const std::string& name, uint32_t amount)
    : game{ game }, self{ self }, other{ other }, name{ name }, amount{ amount }
{}

void CollectItemBehavior::update(float deltaTime)
{
    if (auto s = self.lock(), o = other.lock(); s && o && !done)
    {
        switch (state)
        {
        case 0:
        {
            if (CheckCollisionRecs(s->rect, o->rect))
            {
                game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(name, amount));
                game.playSound("rupee");
                game.eventManager.pushEvent(ITEM_ADDED, name);
                timer = 0.0f;
                state++;
            }
            break;
        }
        case 1:
        {
            timer += deltaTime;
            s->position.x = o->position.x + (o->rect.width - s->rect.width) / 2.0f;
            float offset = std::sin(timer * 10.0f) * 4.0f;
            s->position.y = o->position.y - 20.0f + offset;

            if (timer >= displayDuration)
                state++;
            break;
        }
        default:
        {
            done = true;
            s->markForDeletion();
        }
        }
    }
}

void CollectItemBehavior::reset()
{
    Behavior::reset();
    state = 0;
}
