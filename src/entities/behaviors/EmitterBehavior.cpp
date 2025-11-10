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

    if (auto s = self.lock(); s)
        emitter->location = GetRectCenter(s->rect);
}

void EmitterBehavior::reset() {
    Behavior::reset();

    if (auto s = self.lock(); s && emitter)
        emitter->location = GetRectCenter(s->rect);

    if (emitter)
        emitter->reset();
        emitter->start();
}

void EmitterBehavior::onDeactivate()
{
    if (emitter)
        emitter->stop();
}
