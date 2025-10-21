#include "TeleportBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include <any>

TeleportBehavior::TeleportBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const std::string& targetMap, Vector2 targetPos)
    : game{ game }, self{ self }, other{ other }, targetMap{ targetMap }, targetPos{ targetPos } {
}

void TeleportBehavior::update(float deltaTime) {
    if (auto s = self.lock(), o = other.lock(); s && o && !done) {
        if (CheckCollisionRecs(s->rect, o->rect)) {
            done = true;
            game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [this]() {
                game.eventManager.pushEvent(TELEPORT, std::any(TeleportEvent{ targetMap, targetPos }));
                game.playSound("bookPlace1");
                });
        }
    }
}
