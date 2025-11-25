#include "LungeBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Utils.h"
#include <raymath.h>
#include <cmath>

LungeBehavior::LungeBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, float lungeSpeed, uint32_t jumpForce)
    : game{ game }, self{ self }, target{ target }, lungeSpeed{ lungeSpeed }, jumpForce{ jumpForce }, lungeDirection{ 0.0f, 0.0f }, hasLunged{ false }, originalSpeed{ 0.0f } {
}

void LungeBehavior::update(float deltaTime) {
    if (auto s = self.lock(), t = target.lock(); s && t) {
        if (!hasLunged) {
            Vector2 selfCenter = GetRectCenter(s->rect);
            Vector2 targetCenter = GetRectCenter(t->rect);
            float dx = targetCenter.x - selfCenter.x;
            float dy = targetCenter.y - selfCenter.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist > 0.0f) {
                lungeDirection.x = dx / dist;
                lungeDirection.y = dy / dist;
            }

            s->speed = lungeSpeed;
            s->jump(jumpForce);
            game.playSound("Rise03");
            hasLunged = true;
        }

        s->acc.x = lungeDirection.x;
        s->acc.y = lungeDirection.y;

        if (s->z < 0.0f)
            isAirborne = true;

        if (s->z >= 0.0f && isAirborne) {
            s->acc = { 0.0f, 0.0f };
            s->vel = { 0.0f, 0.0f };
            s->speed = originalSpeed;
            done = true;
            isAirborne = false;
        }
    }
}

void LungeBehavior::draw()
{
    if (game.debug) {
        if (auto s = self.lock(), t = target.lock(); s && t) {
            Vector2 lungeT = { lungeDirection.x * lungeSpeed, lungeDirection.y * lungeSpeed };
            lungeT = Vector2Add(s->position, lungeT);
            DrawLineEx(s->position, lungeT, 1.0f, GREEN);
        }
    }
}

void LungeBehavior::reset() {
    Behavior::reset();
    hasLunged = false;
    lungeDirection = { 0.0f, 0.0f };
    if (auto s = self.lock())
        originalSpeed = s->speed;
}
