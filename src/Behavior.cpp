#include "Behavior.h"
#include "Sprite.h"
#include "Game.h"
#include "InGame.h"
#include "Commands.h"
#include "Controls.h"
#include "Utils.h"
#include "ItemData.h"
#include "Emitter.h"
#include "Particle.h"
#include <any>
#include <cmath>
#include <array>
#include <vector>
#include <limits>


WatchBehavior::WatchBehavior(std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> targetSprite)
    : self{ sprite }, target{ targetSprite } {
}

void WatchBehavior::update(float deltaTime) {
    if (auto s = self.lock(), t = target.lock(); s && t) {
        if (t->position.x < s->position.x) {
            s->lastDirection = LEFT;
        }
        else {
            s->lastDirection = RIGHT;
        }
    }
}

RandomWalkBehavior::RandomWalkBehavior(std::shared_ptr<Sprite> sprite)
    : self{ sprite } {
    if (auto s = self.lock()) {
        walkTarget = s->position;
    }
    hasWalkTarget = false;
}

void RandomWalkBehavior::update(float deltaTime) {
    if (auto s = self.lock()) {
        timer += deltaTime; // Use base class timer

        if (timer < waitTime) {
            return; // Still waiting
        }

        float dx = walkTarget.x - s->position.x;
        float dy = walkTarget.y - s->position.y;
        float distSq = dx * dx + dy * dy;

        if (distSq > 2.0f * 2.0f) {
            float dist = sqrtf(distSq);
            s->acc.x = dx / dist;
            s->acc.y = dy / dist;
        }
        else {
            s->acc = { 0.0f, 0.0f };
            s->vel = { 0.0f, 0.0f };
            hasWalkTarget = false;
        }

        if (!hasWalkTarget) {
            int tries = 20;
            while (tries-- > 0) {
                direction dir = static_cast<direction>(GetRandomValue(RIGHT, DOWN));
                int tiles = GetRandomValue(1, 4); // walk between 1 and 4 tiles at once
                float offset = tiles * 16.0f;
                Vector2 candidate = s->position;
                switch (dir) {
                case UP:
                    candidate.y -= offset;
                    break;
                case LEFT:
                    candidate.x -= offset;
                    break;
                case DOWN:
                    candidate.y += offset;
                    break;
                case RIGHT:
                    candidate.x += offset;
                    break;
                }
                // check if target candidate is within map bounds
                Rectangle testRect = s->rect;
                testRect.x = candidate.x;
                testRect.y = candidate.y;

                if (s->game.isInWorldBounds(testRect) &&
                    isPathClear(s->rect, candidate, s->game.walls)) {
                    walkTarget = candidate;
                    hasWalkTarget = true;
                    waitTime = float(rand() % 5 + 1);
                    timer = 0.0f; // Reset timer for next wait period
                    break;
                }
            }
        }
    }
}

void RandomWalkBehavior::reset() {
    Behavior::reset(); // Call parent reset
    waitTime = 0.0f;
    hasWalkTarget = false;
    if (auto s = self.lock()) {
        walkTarget = s->position;
    }
}

ChaseBehavior::ChaseBehavior(Game& game, std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> targetSprite, float minDist)
    : game{ game }, self{ sprite }, other{ targetSprite }, minDist{ minDist } {
}

void ChaseBehavior::update(float deltaTime) {
    if (auto s = self.lock(), o = other.lock(); s && o) {
        Vector2 selfCenter = GetRectCenter(s->rect);
        Vector2 otherCenter = GetRectCenter(o->rect);
        float dx = otherCenter.x - selfCenter.x;
        float dy = otherCenter.y - selfCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist <= minDist) {
            s->acc = { 0.0f, 0.0f };
            s->vel = { 0.0f, 0.0f };
        }
        else {
            s->acc.x = dx / dist;
            s->acc.y = dy / dist;
        }
    }
    // seperation behavior between enemies
    // TODO: does this scale correctly with deltaTime?
    for (auto& sprite : game.sprites) {
        if (!sprite->isEnemy)
            continue;

        Vector2 sum = { 0, 0 };
        int count = 0;
        float desiredSeparation = sprite->rect.width / 2.0f;

        for (auto& other : game.sprites) {
            if (other != sprite && other->isEnemy) {
                float dx = sprite->position.x - other->position.x;
                float dy = sprite->position.y - other->position.y;
                float distSq = dx * dx + dy * dy;
                if (distSq < desiredSeparation * desiredSeparation) {
                    float dist = std::sqrt(distSq);
                    Vector2 diff = { dx / dist, dy / dist };
                    float mag = 1.0f / dist;
                    diff.x *= mag;
                    diff.y *= mag;
                    sum.x += diff.x;
                    sum.y += diff.y;
                    count++;
                }
            }
        }
        if (count > 0) {
            sum.x /= count;
            sum.y /= count;

            float mag = std::sqrt(sum.x * sum.x + sum.y * sum.y);
            sum.x = sum.x / mag;
            sum.y = sum.y / mag;

            sprite->acc.x += (sum.x - sprite->acc.x);
            sprite->acc.y += (sum.y - sprite->acc.y);
        }
    }
}

WeaponBehavior::WeaponBehavior(Game& game, std::shared_ptr<Sprite> sprite, std::shared_ptr<Sprite> ownerSprite, weaponData data, size_t slot)
    : game{ game }, self{ sprite }, owner{ ownerSprite }, data{ data }, lifetime{ data.lifetime }, originalLifetime{
    data.lifetime }, slot{ slot }
{
    // weaponds with lifetime == -1.0f stay indefinitely
    if (lifetime == -1.0f)
        lifetime = std::numeric_limits<float>::infinity();

    if (auto s = self.lock(), o = owner.lock(); s && o) {
        s->lastDirection = o->lastDirection;
        s->hurtboxOffset.x *= (s->lastDirection == LEFT) ? -1.0f : 1.0f;
        // the player character is left handed; change the drawing order of the weapon accordingly
        s->drawLayer = (s->lastDirection == LEFT) ? 1 : -1;
    }
}

// define the keys to activate the weapon slots
const int WeaponBehavior::controlBindings[2] = { CONTROL_ACTION2, CONTROL_ACTION4 };

void WeaponBehavior::update(float deltaTime) {
    if (auto s = self.lock(), o = owner.lock(); s && o) {
        lifetime -= deltaTime;
        // show the weapon sprite for a split second longer than the lifetime
        if (lifetime < originalLifetime * -0.2f && !done) {
            int eventKey = EventKeyRegistry::getIndexedEventKey(KILL_WEAPON, slot);
            s->game.eventManager.pushEvent(eventKey);
            done = true;
            if (data.onDestroy)
                data.onDestroy(); // callback when weapon is done
        }
        s->position.x = o->position.x + (data.posOffsetX * ((o->lastDirection == RIGHT) ? 1.0f : -1.0f));
        s->position.y = o->position.y - 8.0f + o->z + data.posOffsetY; // factor in the z position
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
                // player jumps
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
            // weapon disappears when the button is pressed again
            if (game.buttonsPressed & controlBindings[slot]) {
                lifetime = 0.0f;
            }

            if (!switchedOn) {
                // execute callback once
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
    Behavior::reset(); // Call parent reset
    lifetime = originalLifetime;
    if (lifetime == -1.0f)
        lifetime = std::numeric_limits<float>::infinity();
    switchedOn = false;
    shaken = false;
}

DeathBehavior::DeathBehavior(Game& game, std::shared_ptr<Sprite> sprite, float lifetime)
    : game{ game }, self{ sprite }, lifetime{ lifetime }, maxLifetime{ lifetime } {
    if (auto s = self.lock()) {
        shader = &s->game.loader.getShader("crumble");
        game.playSound("creature_die_01");
    }
}

void DeathBehavior::update(float deltaTime) {
    if (auto s = self.lock(); s && !done) {
        float elapsed = maxLifetime - lifetime;
        s->activeShader = ShaderState{ shader, elapsed, maxLifetime, 1 };
        lifetime -= deltaTime;
        if (lifetime < 0.0f) {
            done = true;
            s->visible = false;
        }
    }
}

void DeathBehavior::reset() {
    Behavior::reset(); // Call parent reset
    lifetime = maxLifetime;
}

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

HealBehavior::HealBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, uint32_t amount)
    : game{ game }, self{ self }, other{ other }, amount{ amount } {
}

void HealBehavior::update(float deltaTime) {
    if (auto s = self.lock(), o = other.lock(); s && o && !done) {
        if (CheckCollisionRecs(s->rect, o->rect)) {
            done = true;
            // add the amount to health, cap at maxHealth
            o->health = std::min(o->health + amount, o->maxHealth);
            // play sound
            game.playSound("heart");
            // delete this item
            s->markForDeletion();
        }
    }
}

CollectItemBehavior::CollectItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> other, const std::string& name, uint32_t amount)
    : game{ game }, self{ self }, other{ other }, name{ name }, amount{ amount } {
}

void CollectItemBehavior::update(float deltaTime) {
    if (auto s = self.lock(), o = other.lock(); s && o && !done) {
        switch (state) {
        case 0:
        {
            // check collision and collect the item
            if (CheckCollisionRecs(s->rect, o->rect)) {
                // add the item to the inventory if it isn't used immediately
                // (the IMMEDIATE) case is handled within the ADD_ITEM listener because that's when the type is exposed
                game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(name, amount));
                game.playSound("rupee"); // TODO get the correct sound key from data
                game.eventManager.pushEvent(ITEM_ADDED, name);
                timer = 0.0f; // Reset timer for display phase
                state++;
            }
            break;
        }
        case 1: {
            // display the item above the player
            timer += deltaTime; // Use base class timer
            s->position.x = o->position.x + (o->rect.width - s->rect.width) / 2.0f;
            // oscillate the y position slightly
            float offset = std::sin(timer * 10.0f) * 4.0f;
            s->position.y = o->position.y - 20.0f + offset;

            if (timer >= displayDuration) {
                state++;
            }
            break;
        }
        default:
        {
            // delete the item sprite
            done = true;
            s->markForDeletion();
        }
        }
    }
}

void CollectItemBehavior::reset() {
    Behavior::reset(); // Call parent reset
    state = 0;
}

DialogueBehavior::DialogueBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::vector<std::string> dialogTexts, std::string voice)
    : game{ game }, self{ self }, player{ player }, dialogTexts{ std::move(dialogTexts) }, voice{ voice } {
}

void DialogueBehavior::update(float deltaTime) {
    if (triggered)
        return;
    if (auto s = self.lock(), p = player.lock(); s && p) {
        if (CheckCollisionRecs(s->rect, p->rect)) {
            if (!collided) {
                game.eventManager.pushEvent(SHOW_HELP_TEXT, std::make_any<std::tuple<std::string, char, int>>(std::tuple<std::string, char, int>{"TALK", 'O', 9}));
                collided = true;
            }
            if (game.buttonsDown & CONTROL_ACTION1 && !Command_Textbox::isTextboxCooldown()) {
                triggered = true;
                // TODO: why is this check needed again?
                if (auto scene = dynamic_cast<InGame*>(game.getScene("InGame"))) {
                    bool pitch = (voice == "tone") ? false : true;
                    game.cutsceneManager.queueCommand(new Command_Textbox(game, dialogTexts[currentTextIndex], voice, pitch));
                    game.cutsceneManager.queueCommand(new Command_Callback([this]() {
                        game.eventManager.pushDelayedEvent(UNNAMED, 0.3f, nullptr, [this]() {
                            if (currentTextIndex < dialogTexts.size() - 1)
                                ++currentTextIndex;
                            triggered = false;
                            });
                        }));
                }
            }
        }
        else {
            if (collided) {
                // hide the help text if it was previously activated
                collided = false;
                game.eventManager.pushEvent(HIDE_HELP_TEXT);
            }
        }
    }
}

void DialogueBehavior::reset() {
    Behavior::reset(); // Call parent reset
    triggered = false;
    collided = false;
    currentTextIndex = 0;
}

TradeItemBehavior::TradeItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::string name, uint32_t price)
    : game{ game }, self{ self }, player{ player }, name{ name }, price{ price } {
}

void TradeItemBehavior::update(float deltaTime) {
    if (triggered)
        return;
    if (auto s = self.lock(), p = player.lock(); s && p) {
        if (CheckCollisionRecs(s->rect, p->rect)) {
            // show the coin amount
            if (!collided) {
                game.eventManager.pushEvent(SHOW_COIN_AMOUNT);
                collided = true;
            }
            if (game.buttonsDown & CONTROL_ACTION1) {
                triggered = true;
                // player stands next to the item and tries to buy it

                // check if the item can be afforded
                uint32_t qty = game.inventory.getItemQuantity("coin");

                if (qty >= price) {
                    game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(name, 1));
                    game.eventManager.pushEvent(REMOVE_ITEM, std::make_any<std::pair<std::string, uint32_t>>("coin", price));
                    done = true;
                    game.playSound("cash");
                    game.cutsceneManager.queueCommand(new Command_Textbox(game, "Thanks for your purchase."));
                    game.cutsceneManager.queueCommand(new Command_Callback([this]() {
                        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [this]() {
                            triggered = false;
                            });
                        }));
                }
                else {
                    game.cutsceneManager.queueCommand(new Command_Textbox(game, "You can't afford this item."));
                    game.cutsceneManager.queueCommand(new Command_Callback([this]() {
                        // "de-bounce" the interaction by delaying the "triggered" flag
                        game.eventManager.pushDelayedEvent(UNNAMED, 0.2f, nullptr, [this]() {
                            triggered = false;
                            });
                        }));
                }
            }
        }
        else {
            if (collided) {
                collided = false;
                done = false;
                game.eventManager.pushEvent(HIDE_COIN_AMOUNT);
            }
        }
    }
}

void TradeItemBehavior::reset() {
    Behavior::reset(); // Call parent reset
    triggered = false;
    collided = false;
}

void TradeItemBehavior::draw() {
    // draw the coin amount needed to buy this item
    if (auto s = self.lock()) {
        int x = (int)s->position.x - 4;
        int y = (int)s->position.y + 16;
        const auto& coinTex = game.loader.getTextures("itemDropCoin")[0];
        std::string priceText = "x" + std::to_string(price);
        int textW = MeasureText(priceText.c_str(), 10);
        int rectW = coinTex.width + 2 + textW;
        int rectH = std::max(coinTex.height, 10);
        DrawRectangle(x, y, rectW, rectH, Color{ 0, 0, 0, 128 });
        DrawTexture(coinTex, x, y, WHITE);
        DrawText(priceText.c_str(), x + 8, y, 10, WHITE);
    }
}

ProjectileBehavior::ProjectileBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, bool steer, std::optional<Vector2> customDirection)
    : game{ game }, self{ self }, target{ target }, steer{ steer }
{
    if (customDirection.has_value()) {
        // Use the provided direction
        direction = customDirection.value();
        float length = std::sqrt(direction.x * direction.x + direction.y * direction.y);
        if (length > 0.0f) {
            direction.x /= length;
            direction.y /= length;
        }
    }
    else if (!steer && self && target) {
        // Calculate direction from self to target
        Vector2 selfCenter = GetRectCenter(self->rect);
        Vector2 targetCenter = GetRectCenter(target->rect);
        float dx = targetCenter.x - selfCenter.x;
        float dy = targetCenter.y - selfCenter.y;
        float dist = sqrtf(dx * dx + dy * dy);
        direction = { dx / dist, dy / dist };
    }
    self->isColliding = false; // prevents collision separation by the InGame scene
}

void ProjectileBehavior::update(float deltaTime) {
    if (auto s = self.lock(), t = target.lock(); s && t) {
        // check if the projectile hit a wall
        for (const auto& wall : game.walls) {
            if (wall->layer == 0 && CheckCollisionRecs(s->rect, wall->getRect())) {
                s->markForDeletion();
                return;
            }
        }
        // check if the target was hit
        if (CheckCollisionRecs(s->rect, t->rect)) {
            s->markForDeletion();
            return;
        }
        if (steer) {
            // TODO: only steer a little bit towards the target
            Vector2 selfCenter = GetRectCenter(s->rect);
            Vector2 targetCenter = GetRectCenter(t->rect);
            float dx = targetCenter.x - selfCenter.x;
            float dy = targetCenter.y - selfCenter.y;
            float dist = sqrtf(dx * dx + dy * dy);
            s->acc.x = dx / dist;
            s->acc.y = dy / dist;
        }
        else {
            s->acc = direction;
        }
    }
}

ShootBehavior::ShootBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config)
    : game{ game }, self{ self }, target{ target }, config{ config } {
    interval = config.shootInterval;
    // Note: timer starts at 0.0f (from base class), will shoot immediately when timer >= interval
}

void ShootBehavior::update(float deltaTime) {
    timer += deltaTime; // Use base class timer
    if (timer >= interval) {
        timer = 0.0f;
        if (auto s = self.lock(), t = target.lock(); s && t) {
            game.playSound(config.sound);
            Vector2 sCenter = GetRectCenter(s->rect);
            Rectangle sRect = { sCenter.x - config.hitboxSize / 2.0f, sCenter.y - config.hitboxSize / 2.0f, config.hitboxSize, config.hitboxSize }; // center on sprite
            auto projectile = game.createSprite(config.projectileKey, sRect);
            projectile->setTextures({ config.projectileKey, config.projectileKey }); // IDLE and RUN sprites are the same
            projectile->addBehavior(std::make_unique<ProjectileBehavior>(game, projectile, t, false));
            projectile->canHurtPlayer = true;
            projectile->damage = config.damage;
            projectile->speed = config.speed;
            projectile->frameTime = config.frameTime;
            // add a Particle effect that imitates the sprite, but slowly fades
            std::unique_ptr<Emitter> emitter = std::make_unique<Emitter>(config.amount);
            emitter->location = GetRectCenter(s->rect);
            emitter->spawnInterval = config.spawnInterval;
            emitter->lifetimeVariance = config.lifetimeVariance;
            emitter->velocityVariance = config.velocityVariance;
            std::unique_ptr<Particle> proto = std::make_unique<Particle>();
            proto->velocity = config.particleVelocity;
            proto->lifetime = config.particleLifetime;
            proto->alpha = config.particleStartingAlpha;
            proto->endSize = config.particleEndSize;
            proto->setAnimationFrames(game.loader.getTextures(config.projectileKey));
            emitter->prototype = *proto;
            projectile->addBehavior(std::make_unique<EmitterBehavior>(game, projectile, std::move(emitter), std::move(proto)));
        }
    }
}
// ShootBehavior doesn't need reset() override - uses base class timer only

ShootBurstBehavior::ShootBurstBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config, uint32_t burstCount, float burstDelay)
    : game{ game }, self{ self }, target{ target }, config{ config }, burstCount{ burstCount }, shotsFired{ 0 }, burstDelay{ burstDelay } {
}

void ShootBurstBehavior::update(float deltaTime) {
    if (shotsFired >= burstCount) {
        done = true;
        return;
    }

    timer += deltaTime; // Use base class timer
    if (timer >= burstDelay) {
        timer = 0.0f;

        if (auto s = self.lock(), t = target.lock(); s && t) {
            game.playSound(config.sound);
            Vector2 sCenter = GetRectCenter(s->rect);
            Rectangle sRect = { sCenter.x - config.hitboxSize / 2.0f, sCenter.y - config.hitboxSize / 2.0f, config.hitboxSize, config.hitboxSize };
            auto projectile = game.createSprite(config.projectileKey, sRect);
            projectile->setTextures({ config.projectileKey, config.projectileKey });
            projectile->addBehavior(std::make_unique<ProjectileBehavior>(game, projectile, t, false));
            projectile->canHurtPlayer = true;
            projectile->damage = config.damage;
            projectile->speed = config.speed;
            projectile->frameTime = config.frameTime;

            std::unique_ptr<Emitter> emitter = std::make_unique<Emitter>(config.amount);
            emitter->location = GetRectCenter(s->rect);
            emitter->spawnInterval = config.spawnInterval;
            emitter->lifetimeVariance = config.lifetimeVariance;
            emitter->velocityVariance = config.velocityVariance;
            std::unique_ptr<Particle> proto = std::make_unique<Particle>();
            proto->velocity = config.particleVelocity;
            proto->lifetime = config.particleLifetime;
            proto->alpha = config.particleStartingAlpha;
            proto->endSize = config.particleEndSize;
            proto->setAnimationFrames(game.loader.getTextures(config.projectileKey));
            emitter->prototype = *proto;
            projectile->addBehavior(std::make_unique<EmitterBehavior>(game, projectile, std::move(emitter), std::move(proto)));

            shotsFired++;
        }
    }
}

void ShootBurstBehavior::reset() {
    Behavior::reset(); // Call parent reset
    shotsFired = 0;
}

ShootSpreadBehavior::ShootSpreadBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, shootingConfig config, uint32_t projectileCount, float spreadAngle)
    : game{ game }, self{ self }, target{ target }, config{ config }, projectileCount{ projectileCount }, spreadAngle{ spreadAngle }, hasFired{ false } {
}

void ShootSpreadBehavior::update(float deltaTime) {
    if (hasFired) {
        done = true;
        return;
    }

    if (auto s = self.lock(), t = target.lock(); s && t) {
        game.playSound(config.sound);
        Vector2 sCenter = GetRectCenter(s->rect);
        Vector2 tCenter = GetRectCenter(t->rect);

        float baseAngle = std::atan2(tCenter.y - sCenter.y, tCenter.x - sCenter.x);
        float startAngle = baseAngle - spreadAngle / 2.0f;
        float angleStep = (projectileCount > 1) ? spreadAngle / (projectileCount - 1) : 0.0f;

        for (uint32_t i = 0; i < projectileCount; i++) {
            float currentAngle = startAngle + angleStep * i;

            Rectangle sRect = { sCenter.x - config.hitboxSize / 2.0f, sCenter.y - config.hitboxSize / 2.0f, config.hitboxSize, config.hitboxSize };
            auto projectile = game.createSprite(config.projectileKey, sRect);
            projectile->setTextures({ config.projectileKey, config.projectileKey });
            projectile->canHurtPlayer = true;
            projectile->damage = config.damage;
            projectile->speed = config.speed;
            projectile->frameTime = config.frameTime;
            projectile->isColliding = false;

            Vector2 direction = { std::cos(currentAngle), std::sin(currentAngle) };
            projectile->acc = direction;
            projectile->addBehavior(std::make_unique<ProjectileBehavior>(game, projectile, t, false, direction));

            std::unique_ptr<Emitter> emitter = std::make_unique<Emitter>(config.amount);
            emitter->location = sCenter;
            emitter->spawnInterval = config.spawnInterval;
            emitter->lifetimeVariance = config.lifetimeVariance;
            emitter->velocityVariance = config.velocityVariance;
            std::unique_ptr<Particle> proto = std::make_unique<Particle>();
            proto->velocity = config.particleVelocity;
            proto->lifetime = config.particleLifetime;
            proto->alpha = config.particleStartingAlpha;
            proto->endSize = config.particleEndSize;
            proto->setAnimationFrames(game.loader.getTextures(config.projectileKey));
            emitter->prototype = *proto;
            projectile->addBehavior(std::make_unique<EmitterBehavior>(game, projectile, std::move(emitter), std::move(proto)));
        }

        hasFired = true;
    }
}

void ShootSpreadBehavior::reset() {
    Behavior::reset(); // Call parent reset
    hasFired = false;
}

KiteBehavior::KiteBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> target, float orbitDistance, float moveSpeed)
    : game{ game }, self{ self }, target{ target }, orbitDistance{ orbitDistance }, moveSpeed{ moveSpeed }, orbitAngle{ 0.0f } {
    orbitAngle = static_cast<float>(GetRandomValue(0, 628)) / 100.0f;
}

void KiteBehavior::update(float deltaTime) {
    if (auto s = self.lock(), t = target.lock(); s && t) {
        Vector2 targetCenter = GetRectCenter(t->rect);

        orbitAngle += moveSpeed * deltaTime;
        if (orbitAngle > 2.0f * PI)
            orbitAngle -= 2.0f * PI;

        desiredPos.x = targetCenter.x + std::cos(orbitAngle) * orbitDistance;
        desiredPos.y = targetCenter.y + std::sin(orbitAngle) * orbitDistance;

        Vector2 selfCenter = GetRectCenter(s->rect);
        float dx = desiredPos.x - selfCenter.x;
        float dy = desiredPos.y - selfCenter.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist > 2.0f) {
            s->acc.x = dx / dist;
            s->acc.y = dy / dist;
        }
        else {
            s->acc = { 0.0f, 0.0f };
        }

        if (desiredPos.x < selfCenter.x)
            s->lastDirection = LEFT;
        else
            s->lastDirection = RIGHT;
    }
}

void KiteBehavior::draw()
{
    if (game.debug) {
        DrawCircle(static_cast<int>(desiredPos.x), static_cast<int>(desiredPos.y), 2.0f, RED);
        if (auto s = self.lock(); s) {
            DrawLineEx(s->position, desiredPos, 1.0f, RED);
        }
    }
}

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
            //s->currentAnimState = CHARGE;  // TODO: does not work here bc it gets overwritten in Sprite.animate()
            game.playSound("Rise03");
            hasLunged = true;
        }

        s->acc.x = lungeDirection.x;
        s->acc.y = lungeDirection.y;

        if (s->z < 0.0f)
            isAirborne = true;

        if (s->z >= 0.0f && isAirborne) {
            // Sprite has landed, reset the attributes
            s->acc = { 0.0f, 0.0f };
            s->vel = { 0.0f, 0.0f };
            s->speed = originalSpeed;
            done = true;
            isAirborne = false;
            //s->currentAnimState = IDLE;
        }
    }
}

void LungeBehavior::draw()
{
    if (game.debug) {
        if (auto s = self.lock(), t = target.lock(); s && t) {
            Vector2 lungeT = { lungeDirection.x * lungeSpeed, lungeDirection.y * lungeSpeed };
            lungeT = Vector2Add(s->position, lungeT); // TODO does not show the lunge vector correctly
            DrawLineEx(s->position, lungeT, 1.0f, GREEN);
        }
    }
}

void LungeBehavior::reset() {
    Behavior::reset(); // Call parent reset
    hasLunged = false;
    lungeDirection = { 0.0f, 0.0f };
    if (auto s = self.lock())
        originalSpeed = s->speed;
}

EmitterBehavior::EmitterBehavior(Game& game, std::shared_ptr<Sprite> self, std::unique_ptr<Emitter> emitter, std::unique_ptr<Particle> prototype) : game{ game }, self{ self }, emitter{ std::move(emitter) }, prototype{ std::move(prototype) } {}

void EmitterBehavior::update(float deltaTime) {
    if (auto s = self.lock(); s) {
        emitter->location = GetRectCenter(s->rect);
    }
    emitter->update(deltaTime);
}

void EmitterBehavior::draw() {
    emitter->draw();
}

void EmitterBehavior::reset()
{
    // TODO position glitches out when pausing this behavior
    Behavior::reset(); // Call parent reset

    // Update emitter location to current sprite position immediately
    if (auto s = self.lock(); s)
        emitter->location = GetRectCenter(s->rect);

    emitter->reset(); // Clear old particles and reset timers
}

ChestBehavior::ChestBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, const std::string& itemName, uint32_t itemAmount) : game{ game }, self{ self }, player{ player }, itemName{ itemName }, itemAmount{ itemAmount }
{
}

void ChestBehavior::update(float deltaTime) {
    if (auto s = self.lock(), p = player.lock(); s && p) {
        interactionRect.x = s->rect.x;
        interactionRect.y = s->rect.y;
        interactionRect.width = s->rect.width;
        interactionRect.height = s->rect.height + 4.0f;
        if (CheckCollisionRecs(interactionRect, p->rect)) {
            if (triggered) // check this down here because the check for the HIDE_HELP_TEXT event always needs to happen
                return;
            if (!collided) {
                game.eventManager.pushEvent(SHOW_HELP_TEXT, std::make_any<std::tuple<std::string, char, int>>(std::tuple<std::string, char, int>{"OPEN", 'O', 9}));
                collided = true;
            }
            if (game.buttonsDown & CONTROL_ACTION1) {
                // opens the chest
                triggered = true;
                auto& itemData = game.inventory.getItemData();
                const ItemData& data = itemData.at(itemName);
                s->currentFrame = 2;
                showItem = true;
                game.playSound("doorOpen_2");
                game.eventManager.pushDelayedEvent(UNNAMED, 2.0f, nullptr, [&]() {
                    showItem = false;
                    });

                game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.playSound("Rise03");
                    }));
                game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
                std::string message;
                if (itemAmount == 1) {
                    // TODO: add a "unique" property to Item that is checked here instead
                    if (data.type == WEAPON) {
                        message = format("You got the %s.\nOpen your inventory to equip it, then use with [P].", data.displayName.c_str());
                    }
                    else {
                        message = format("You got a %s.", data.displayName.c_str());
                    }
                }
                else {
                    message = format("You got: %s x%u", data.displayName.c_str(), itemAmount);
                }
                game.cutsceneManager.queueCommand(new Command_Textbox(game, message));
                // event that adds the item to the inventory
                game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(itemName, itemAmount));
                // trigger the event that changes the object state
                std::string eventStr = "chest_opened_" + std::to_string(s->tileMapID);
                int eventKey = EventKeyRegistry::getEventKey(eventStr);
                game.eventManager.pushEvent(eventKey, s->tileMapID);
            }
        }
        else {
            if (collided) {
                game.eventManager.pushEvent(HIDE_HELP_TEXT);
                collided = false;
            }
        }
    }
}

void ChestBehavior::draw() {
    if (!showItem)
        return;
    if (auto s = self.lock()) {
        // draw the item that comes out of the chest
        int x = (int)s->position.x;
        int y = (int)s->position.y - 16;
        auto& itemData = game.inventory.getItemData();
        const ItemData& data = itemData.at(itemName);
        const auto& textures = game.loader.getTextures(data.textureKey);
        if (textures.size() == 0) {
            TraceLog(LOG_ERROR, "No texture found for %s", data.textureKey.c_str());
            return;
        }
        // adjust x position to account for item texture width
        int item_tex_width = textures[0].width;
        int chest_tex_width = s->frames[s->currentAnimState][s->currentFrame].width;
        x += (chest_tex_width - item_tex_width) / 2;
        DrawTexture(textures[0], x, y, WHITE);
    }
}

void ChestBehavior::reset() {
    Behavior::reset(); // Call parent reset
    triggered = false;
    collided = false;
    showItem = false;
}

OpenLockBehavior::OpenLockBehavior(Game& game, std::shared_ptr<Sprite> door, std::shared_ptr<Sprite> player, const int triggerKey)
    : game{ game }, door{ door }, player{ player }, triggerKey{ triggerKey } {
}

void OpenLockBehavior::update(float deltaTime) {
    if (triggered) return;
    if (auto d = door.lock(), p = player.lock(); d && p) {
        const float padding = 2.0f; // the interaction rect is inflated by a few pixels 
        interactionRect.x = d->rect.x - padding;
        interactionRect.y = d->rect.y - padding;
        interactionRect.width = d->rect.width + 2.0f * padding;
        interactionRect.height = d->rect.height + 2.0f * padding;
        if (CheckCollisionRecs(interactionRect, p->rect)) {
            if (!collided) {
                game.eventManager.pushEvent(SHOW_HELP_TEXT, std::make_any<std::tuple<std::string, char, int>>(std::tuple<std::string, char, int>{"OPEN", 'O', 9}));
                collided = true;
            }
            if (game.buttonsDown & CONTROL_ACTION1) {
                // check for keys
                uint32_t qty = game.inventory.getItemQuantity("key");
                triggered = true;
                if (qty == 0) {
                    game.cutsceneManager.queueCommand(new Command_Textbox(game, "Looks like you need a key to open this door."));
                    game.cutsceneManager.queueCommand(new Command_Callback([this]() {
                        // "de-bounce" the interaction by delaying the "triggered" flag
                        game.eventManager.pushDelayedEvent(UNNAMED, 0.2f, nullptr, [this]() {
                            triggered = false;
                            });
                        }));
                    return;
                }
                game.eventManager.pushEvent(REMOVE_ITEM, std::make_any<std::pair<std::string, uint32_t>>("key", 1));
                game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [d, this]() {
                    this->game.playSound("bookPlace1");
                    d->currentFrame = 0; // remove the lock
                    this->game.eventManager.pushEvent(triggerKey); // triggers a change in the persistent room data
                    });
                game.eventManager.pushDelayedEvent(UNNAMED, 0.8f, nullptr, [d, this]() {
                    this->game.playSound("doorOpen_2");
                    d->currentFrame = 1; // show the open door
                    d->staticCollision = false;
                    this->done = true;
                    });
            }
        }
        else {
            if (collided) {
                game.eventManager.pushEvent(HIDE_HELP_TEXT);
                collided = false;
            }
        }
    }
}

void OpenLockBehavior::reset() {
    Behavior::reset(); // Call parent reset
    triggered = false;
    collided = true; // Note: starts as true in original code
}


std::unique_ptr<Behavior> createBehaviorFromJSON(Game& game, std::shared_ptr<Sprite> sprite, const std::string& behaviorKey, const nlohmann::json& behaviorData)
{
    if (behaviorKey == "RandomWalk") {
        return std::make_unique<RandomWalkBehavior>(sprite);
    }
    else if (behaviorKey == "Watch") {
        std::string targetName = behaviorData.value("watchTarget", "");
        if (game.spriteMap.find(targetName) != game.spriteMap.end()) {
            return std::make_unique<WatchBehavior>(sprite, game.spriteMap[targetName]);
        }
        else {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping WatchBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "Chase") {
        std::string targetName = behaviorData.value("chaseTarget", "player");
        float minDist = behaviorData.value("minDist", 20.0f);

        if (game.spriteMap.find(targetName) != game.spriteMap.end()) {
            return std::make_unique<ChaseBehavior>(game, sprite, game.spriteMap[targetName], minDist);
        }
        else {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ChaseBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "Dialogue") {
        std::string textKey = behaviorData.value("dialogue", "");
        if (textKey.length()) {
            std::vector<std::string> texts = game.loader.getText(textKey);
            std::string voice = behaviorData.value("voice", "tone");
            return std::make_unique<DialogueBehavior>(game, sprite, game.spriteMap["player"], texts, voice);
        }
        return nullptr;
    }
    else if (behaviorKey == "Shoot") {
        std::string targetName = behaviorData.value("shootTarget", "player");
        shootingConfig conf;
        conf.projectileKey = behaviorData.value("shootProjectile", conf.projectileKey);
        conf.sound = behaviorData.value("shootSound", conf.sound);
        conf.damage = behaviorData.value("shootDamage", conf.damage);
        conf.shootInterval = behaviorData.value("shootInterval", 2.0f);
        conf.speed = behaviorData.value("shootSpeed", 20.0f);
        conf.amount = 10;
        conf.velocityVariance = { 1.0f, 1.0f };
        conf.spawnInterval = 0.1f;
        conf.lifetimeVariance = 0.2f;

        if (game.spriteMap.find(targetName) != game.spriteMap.end()) {
            return std::make_unique<ShootBehavior>(game, sprite, game.spriteMap[targetName], conf);
        }
        else {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ShootBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "Emitter") {
        // Get the emitter key from behavior data
        std::string emitterKey = behaviorData.value("emitter", behaviorData.value("particle", ""));
        if (emitterKey.empty()) {
            TraceLog(LOG_WARNING, "No emitter/particle key specified for Emitter behavior");
            return nullptr;
        }

        // Load particles.json data
        const auto& particlesData = game.loader.getParticleData(); // TODO: change to getParticleData() when implemented

        // Get the default emitter and particle for fallback values
        if (particlesData.find("defaultEmitter") == particlesData.end() ||
            particlesData.find("defaultParticle") == particlesData.end()) {
            TraceLog(LOG_ERROR, "Missing 'defaultEmitter' or 'defaultParticle' in particles.json");
            return nullptr;
        }

        const auto& defaultEmitterData = particlesData.at("defaultEmitter");
        const auto& defaultParticleData = particlesData.at("defaultParticle");

        // Get specific emitter data
        if (particlesData.find(emitterKey) == particlesData.end()) {
            TraceLog(LOG_WARNING, "Emitter key \"%s\" not found in particles.json", emitterKey.c_str());
            return nullptr;
        }

        const auto& emitterData = particlesData.at(emitterKey);

        // Get particle prototype key
        std::string particleKey = emitterData.value("particleKey", defaultEmitterData.value("particleKey", "defaultParticle"));
        if (particlesData.find(particleKey) == particlesData.end()) {
            TraceLog(LOG_WARNING, "Particle key \"%s\" not found in particles.json", particleKey.c_str());
            return nullptr;
        }

        const auto& particleData = particlesData.at(particleKey);

        // Create emitter with settings from JSON (with defaults as fallback)
        size_t maxParticles = emitterData.value("maxParticles", defaultEmitterData.value("maxParticles", 20));
        std::unique_ptr<Emitter> emitter = std::make_unique<Emitter>(maxParticles);

        emitter->spawnInterval = emitterData.value("spawnInterval", defaultEmitterData.value("spawnInterval", 1.0f));
        emitter->emitterLifetime = emitterData.value("emitterLifetime", defaultEmitterData.value("emitterLifetime", -1.0f));
        emitter->spawnRadius = emitterData.value("spawnRadius", defaultEmitterData.value("spawnRadius", 0.0f));
        emitter->spawnRadiusVariance = emitterData.value("spawnRadiusVariance", defaultEmitterData.value("spawnRadiusVariance", 0.0f));
        emitter->velocityVariance.x = emitterData.value("velocityVarianceX", defaultEmitterData.value("velocityVarianceX", 0.0f));
        emitter->velocityVariance.y = emitterData.value("velocityVarianceY", defaultEmitterData.value("velocityVarianceY", 0.0f));
        emitter->lifetimeVariance = emitterData.value("lifetimeVariance", defaultEmitterData.value("lifetimeVariance", 0.0f));
        emitter->alphaVariance = emitterData.value("alphaVariance", defaultEmitterData.value("alphaVariance", 0.0f));
        emitter->radialVelocity = emitterData.value("radialVelocity", defaultEmitterData.value("radialVelocity", false));
        emitter->speed = emitterData.value("speed", defaultEmitterData.value("speed", 1.0f));
        emitter->speedVariance = emitterData.value("speedVariance", defaultEmitterData.value("speedVariance", 0.0f));

        // Create particle prototype from JSON (with defaults as fallback)
        std::unique_ptr<Particle> proto = std::make_unique<Particle>();

        proto->velocity.x = particleData.value("velocityX", defaultParticleData.value("velocityX", 0.0f));
        proto->velocity.y = particleData.value("velocityY", defaultParticleData.value("velocityY", 0.0f));
        proto->startAlpha = particleData.value("startAlpha", defaultParticleData.value("startAlpha", 1.0f));
        proto->endAlpha = particleData.value("endAlpha", defaultParticleData.value("endAlpha", 0.0f));
        proto->lifetime = particleData.value("lifetime", defaultParticleData.value("lifetime", 1.0f));
        proto->startSize = particleData.value("startSize", defaultParticleData.value("startSize", 1.0f));
        proto->endSize = particleData.value("endSize", defaultParticleData.value("endSize", 1.0f));
        proto->animationSpeed = particleData.value("animationSpeed", defaultParticleData.value("animationSpeed", 0.1f));

        // Handle tint color (with default fallback)
        if (particleData.contains("tint") || defaultParticleData.contains("tint")) {
            auto tintArray = particleData.contains("tint") ? particleData.at("tint") : defaultParticleData.at("tint");
            proto->tint = Color{
                static_cast<unsigned char>(tintArray[0].get<int>()),
                static_cast<unsigned char>(tintArray[1].get<int>()),
                static_cast<unsigned char>(tintArray[2].get<int>()),
                static_cast<unsigned char>(tintArray[3].get<int>())
            };
        }

        // Set animation frames from texture key (with default fallback)
        std::string textureKey = particleData.value("textureKey", defaultParticleData.value("textureKey", "sprite_default"));
        proto->setAnimationFrames(game.loader.getTextures(textureKey));

        emitter->prototype = *proto;
        return std::make_unique<EmitterBehavior>(game, sprite, std::move(emitter), std::move(proto));
    }
    if (behaviorKey == "Kite") {
        std::string targetName = behaviorData.value("kiteTarget", "player");
        float orbitDistance = behaviorData.value("orbitDistance", 80.0f);
        float moveSpeed = behaviorData.value("moveSpeed", 1.5f);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
            return std::make_unique<KiteBehavior>(game, sprite, game.spriteMap[targetName], orbitDistance, moveSpeed);
        else {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping KiteBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "ShootBurst") {
        std::string targetName = behaviorData.value("shootTarget", "player");
        shootingConfig conf;
        conf.projectileKey = behaviorData.value("shootProjectile", conf.projectileKey);
        conf.sound = behaviorData.value("shootSound", conf.sound);
        conf.damage = behaviorData.value("shootDamage", conf.damage);
        conf.speed = behaviorData.value("shootSpeed", 20.0f);
        conf.amount = 10;
        conf.velocityVariance = { 1.0f, 1.0f };
        conf.spawnInterval = 0.1f;
        conf.lifetimeVariance = 0.2f;
        uint32_t burstCount = behaviorData.value("burstCount", 3);
        float burstDelay = behaviorData.value("burstDelay", 0.3f);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
            return std::make_unique<ShootBurstBehavior>(game, sprite, game.spriteMap[targetName], conf, burstCount, burstDelay);
        else {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ShootBurstBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "ShootSpread") {
        std::string targetName = behaviorData.value("shootTarget", "player");
        shootingConfig conf;
        conf.projectileKey = behaviorData.value("shootProjectile", conf.projectileKey);
        conf.sound = behaviorData.value("shootSound", conf.sound);
        conf.damage = behaviorData.value("shootDamage", conf.damage);
        conf.speed = behaviorData.value("shootSpeed", 20.0f);
        conf.amount = 10;
        conf.velocityVariance = { 1.0f, 1.0f };
        conf.spawnInterval = 0.1f;
        conf.lifetimeVariance = 0.2f;
        uint32_t projectileCount = behaviorData.value("projectileCount", 3);
        float spreadAngle = behaviorData.value("spreadAngle", 0.5f);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
            return std::make_unique<ShootSpreadBehavior>(game, sprite, game.spriteMap[targetName], conf, projectileCount, spreadAngle);
        else {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping ShootSpreadBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else if (behaviorKey == "Lunge") {
        std::string targetName = behaviorData.value("lungeTarget", "player");
        float lungeSpeed = behaviorData.value("lungeSpeed", 30.0f);
        uint32_t jumpForce = behaviorData.value("jumpForce", 600);

        if (game.spriteMap.find(targetName) != game.spriteMap.end())
            return std::make_unique<LungeBehavior>(game, sprite, game.spriteMap[targetName], lungeSpeed, jumpForce);
        else {
            TraceLog(LOG_WARNING, "Target \"%s\" not found in spriteMap. Skipping LungeBehavior.", targetName.c_str());
            return nullptr;
        }
    }
    else {
        TraceLog(LOG_WARNING, "Unknown behavior type: %s", behaviorKey.c_str());
        return nullptr;
    }
}


void addBehaviorsToSprite(Game& game, std::shared_ptr<Sprite> sprite, const std::vector<std::string>& behaviors, const nlohmann::json& behaviorData) {
    for (const auto& key : behaviors) {
        auto behavior = createBehaviorFromJSON(game, sprite, key, behaviorData);
        if (behavior)
            sprite->addBehavior(std::move(behavior));
    }
}