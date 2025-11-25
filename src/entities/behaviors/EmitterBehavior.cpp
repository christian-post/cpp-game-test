#include "EmitterBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include "Emitter.h"

EmitterBehavior::EmitterBehavior(Game& game, std::shared_ptr<Sprite> self, std::unique_ptr<Emitter> emitter)
    : game{ game }, self{ self } {
    this->emitter = emitter.get();
    game.emitters.push_back(std::move(emitter));
}

void EmitterBehavior::update(float deltaTime) {
    if (!emitter)
        return;

    if (auto s = self.lock(); s) {
        emitter->position = GetRectCenter(s->rect);
        //TraceLog(LOG_INFO, "(%d, %d)", int(emitter->position.x), int(emitter->position.y));
    }
        
}

void EmitterBehavior::reset() {
    Behavior::reset();

    if (auto s = self.lock(); s && emitter)
        emitter->position = GetRectCenter(s->rect);

    if (emitter)
        emitter->reset();
        emitter->start();
}

void EmitterBehavior::onDeactivate()
{
    if (emitter)
        emitter->stop();
}
