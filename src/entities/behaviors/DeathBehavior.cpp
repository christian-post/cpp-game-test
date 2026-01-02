#include "DeathBehavior.h"
#include "Sprite.h"
#include "Game.h"

DeathBehavior::DeathBehavior(Game& game, std::shared_ptr<Sprite> sprite, float lifetime)
    : game{ game }, self{ sprite }, lifetime{ lifetime }, maxLifetime{ lifetime }
{
    if (auto s = self.lock())
    {
        shader = &s->game.loader.getShader("crumble");
        game.playSound("creature_die_01");
    }
}

void DeathBehavior::update(float deltaTime)
{
    if (auto s = self.lock(); s && !done)
    {
        float elapsed = maxLifetime - lifetime;
        s->activeShader = ShaderState{ shader, elapsed, maxLifetime, 1 };
        lifetime -= deltaTime;
        if (lifetime < 0.0f)
        {
            done = true;
            s->visible = false;
        }
    }
}

void DeathBehavior::reset()
{
    Behavior::reset();
    lifetime = maxLifetime;
}
