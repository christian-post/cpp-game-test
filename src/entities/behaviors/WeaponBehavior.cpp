#include "WeaponBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "InGame.h"
#include "Commands.h"
#include "Controls.h"
#include <cmath>
#include <limits>

const int WeaponBehavior::controlBindings[2] = { CONTROL_ACTION2, CONTROL_ACTION3 };

WeaponBehavior::WeaponBehavior(Game& game, std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> ownerSprite, weaponData data, size_t slot)
    : game{ game }, self{ sprite }, owner{ ownerSprite }, data{ data }, lifetime{ data.lifetime }, originalLifetime{ data.lifetime }, slot{ slot }
{
    if (lifetime == -1.0f)
        lifetime = std::numeric_limits<float>::infinity();

    if (auto s = self.lock(), o = owner.lock(); s && o) {
        s->lastDirection = o->lastDirection;
        s->hurtboxOffset.x *= (s->lastDirection == LEFT) ? -1.0f : 1.0f;
        s->drawLayer = (s->lastDirection == LEFT) ? 1 : -1;
    }
}

void WeaponBehavior::update(float deltaTime) {
    if (auto s = self.lock(), o = owner.lock(); s && o) {
        lifetime -= deltaTime;
        if (lifetime < originalLifetime * -0.2f && !done) {
            int eventKey = EventKeyRegistry::getIndexedEventKey(KILL_WEAPON, slot);
            s->game.eventManager.pushEvent(eventKey);
            done = true;
            if (data.onDestroy)
                data.onDestroy();
        }
        s->position.x = o->position.x + (data.posOffsetX * ((o->lastDirection == RIGHT) ? 1.0f : -1.0f));
        s->position.y = o->position.y - 8.0f + o->z + data.posOffsetY;
        float progress = 1.0f - (lifetime / originalLifetime);
        if (lifetime < 0.0f)
            return;
        switch (data.type) {
        case SWING:
            s->rotationAngle = (s->lastDirection == RIGHT) ? 180.0f * progress : -180.0f * progress;
            break;
        case WHACK:
        {
            float angle = std::sin(progress * 3.14159f);
            s->rotationAngle = (s->lastDirection == RIGHT) ? 90.0f * angle : -90.0f * angle;

            if (!shaken && progress > 0.5f) {
                s->game.eventManager.pushEvent(SCREEN_SHAKE, std::make_tuple(0.1f, 0.0f, 10.0f));
                s->game.playSound("hammer");
                o->jump();
                shaken = true;
            }
        }
        break;
        case POKE:
        {
            float offset = std::sin(progress * 3.14159f) * 10.0f;
            if (o->lastDirection == RIGHT) {
                s->position.x += offset;
                s->rotationAngle = 90;
            }
            else {
                s->position.x -= offset;
                s->rotationAngle = -90;
            }
            break;
        }
        case HOLD:
            if (game.buttonsPressed & controlBindings[slot])
                lifetime = 0.0f;

            if (!switchedOn) {
                if (data.onCreate)
                    data.onCreate();
                switchedOn = true;
            }
            break;
        default:
            break;
        }
    }
}

void WeaponBehavior::reset() {
    Behavior::reset();
    lifetime = originalLifetime;
    if (lifetime == -1.0f)
        lifetime = std::numeric_limits<float>::infinity();
    switchedOn = false;
    shaken = false;
}
