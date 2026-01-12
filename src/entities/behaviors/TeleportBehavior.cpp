#include "TeleportBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include <any>

TeleportBehavior::TeleportBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const std::string& targetWorld, size_t targetLevel, size_t targetIndex, Vector2 targetPos)
    : game{ game }, self{ self }, other{ other }, targetWorld{ targetWorld }, targetLevel{ targetLevel }, targetIndex{ targetIndex }, targetPos {
    targetPos
}
{}

void TeleportBehavior::update(float deltaTime) 
{
    if (auto s = self.lock(), o = other.lock(); s && o && !done)
    {
        if (CheckCollisionRecs(s->rect, o->rect))
        {
            done = true;
            game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [this]() {
                game.eventManager.pushEvent(TELEPORT, std::any(TeleportEvent{ targetWorld, targetLevel, targetIndex, targetPos }));
                game.playSound("bookPlace1"); // TODO
                });
        }
    }
}
