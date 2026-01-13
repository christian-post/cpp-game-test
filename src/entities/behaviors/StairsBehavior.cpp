#include "StairsBehavior.h"
#include "Game.h"
#include "Sprite.h"
#include <algorithm>

StairsBehavior::StairsBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const int level) : game{ game }, self{ self }, other{ other }, levelChange{ level }
{}

void StairsBehavior::update(float deltaTime)
{
    if (auto s = self.lock(), o = other.lock(); s && o && !done)
    {
        if (CheckCollisionRecs(s->rect, o->rect))
        {
            if (s->isMarkedForDeletion() || o->isMarkedForDeletion())
                return;

            done = true;
            game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [this]() {
                int change = static_cast<int>(game.currentWorld->currentLevel) + levelChange;
                size_t newLevel = static_cast<size_t>(std::max(0, change)); // ensure positive numbers
                game.currentWorld->currentLevel = newLevel;
                game.eventManager.pushEvent(RELOAD_ROOM);
                //game.playSound("bookPlace1"); // TODO stairs sound
                });
        }
    }
}
