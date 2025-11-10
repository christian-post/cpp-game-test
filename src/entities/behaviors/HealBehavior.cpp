#include "HealBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include <algorithm>

HealBehavior::HealBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, uint32_t amount)
    : game{ game }, self{ self }, other{ other }, amount{ amount } {
}

void HealBehavior::update(float deltaTime) {
    if (auto s = self.lock(), o = other.lock(); s && o && !done) {
        if (CheckCollisionRecs(s->rect, o->rect)) {
            done = true;
            o->health = std::min(o->health + amount, o->maxHealth);
            game.playSound("heart");
            s->markForDeletion();
        }
    }
}
