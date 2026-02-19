#include "WeaponBehavior.h"
#include "ProjectileBehavior.h"
#include "EmitterBehavior.h"
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
    // a -1 indicates weapons that stay indefinitely (until manually unequipped)
    if (lifetime == -1.0f)
        lifetime = std::numeric_limits<float>::infinity();

    if (auto s = self.lock(), o = owner.lock(); s && o)
    {
        // change some variables based on the sprite that holds this weapon
        s->lastDirection = o->lastDirection;
        s->hurtboxOffset.x *= (s->lastDirection == LEFT) ? -1.0f : 1.0f;
        s->drawLayer = (s->lastDirection == LEFT) ? 1 : -1;
    }
}

void WeaponBehavior::update(float deltaTime) 
{
    if (auto s = self.lock(), o = owner.lock(); s && o) 
    {
        if (o->isMarkedForDeletion())
            return;

        lifetime -= deltaTime;
        // weapon is done
        if (lifetime < originalLifetime * -0.2f && !done)
        {
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
        // weapon specific behavior
        switch (data.type)
        {
        case SWING:
            s->rotationAngle = (s->lastDirection == RIGHT) ? 180.0f * progress : -180.0f * progress;
            break;
        case WHACK:
        {
            // does a quarter rotation and then shakes the screen
            float angle = std::sin(progress * 3.14159f);
            s->rotationAngle = (s->lastDirection == RIGHT) ? 90.0f * angle : -90.0f * angle;

            if (!shaken && progress > 0.5f)
            {
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
            if (o->lastDirection == RIGHT)
            {
                s->position.x += offset;
                s->rotationAngle = 90;
            }
            else
            {
                s->position.x -= offset;
                s->rotationAngle = -90;
            }
            break;
        }
        case HOLD:
            if (game.buttonsPressed & controlBindings[slot])
                lifetime = 0.0f;

            if (!switchedOn)
            {
                if (data.onCreate)
                    data.onCreate();
                switchedOn = true;
            }
            break;
        case BOW :
        {
            if (!isNotched && controlBindings[slot])
            {
                // first press enters aim mode
                isNotched = true;
                aimDirection = { o->lastDirection == RIGHT ? 1.0f : -1.0f, 0.0f };  // initialize from current direction
                //o->setAnimState(CHARGE, true); // TODO
                game.eventManager.pushEvent(LOCK_PLAYER_MOVEMENT);
            }
            else if (isNotched)
            {
                timer += deltaTime;

                // update aim direction based on input
                Vector2 inputDir = { 0.0f, 0.0f };
                if (game.buttonsDown & CONTROL_RIGHT)
                    inputDir.x = 1.0f;
                if (game.buttonsDown & CONTROL_LEFT)
                    inputDir.x = -1.0f;
                if (game.buttonsDown & CONTROL_DOWN)
                    inputDir.y = 1.0f;
                if (game.buttonsDown & CONTROL_UP)
                    inputDir.y = -1.0f;

                // if player is inputting a direction, update aim
                if (inputDir.x != 0.0f || inputDir.y != 0.0f)
                {
                    float length = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
                    aimDirection.x = inputDir.x / length;
                    aimDirection.y = inputDir.y / length;
                }

                // update weapon rotation to match aim direction
                s->rotationAngle = std::atan2(aimDirection.y, aimDirection.x) * RAD2DEG;
                // adjust position based on rotation to keep bow at player's hand
                // TODO define all magic numbers at the top
                float offsetX = 0.0f;
                float offsetY = 0.0f;
                if (std::abs(aimDirection.x) > std::abs(aimDirection.y))
                {
                    // horizontal aim
                    if (aimDirection.x > 0.0f)
                    {
                        // aiming RIGHT, do nothing
                    }
                    else
                    {
                        // Aiming LEFT
                        offsetX = -10.0f;
                        offsetY = -1.0f * static_cast<float>(o->frames[o->currentAnimState][o->currentFrame].height);
                    }
                }
                else
                {
                    // vertical aim
                    if (aimDirection.y > 0.0f)
                    {
                        // DOWN
                        offsetX = -16.0f;
                        offsetY = -6.0f;
                    }
                    else
                    {
                        // UP
                        offsetX = 8.0f;
                        offsetY = -18.0f;
                    }
                }
                s->position.x = s->position.x + offsetX;
                s->position.y = s->position.y + offsetY + o->z;

                // second press fires the arrow
                if (game.buttonsPressed & controlBindings[slot])
                {
                    // calculate projectile spawn position using same logic as draw()
                    const auto& weaponTextures = s->frames[s->currentAnimState];
                    float wpnTexHeight = weaponTextures.empty() ? s->rect.height : static_cast<float>(weaponTextures[s->currentFrame].height);

                    // calculate the rotation pivot (center-bottom of weapon texture)
                    Vector2 rotationPivot = {
                        s->position.x + s->rect.width / 2.0f + s->hitboxOffset.x,
                        s->position.y + s->rect.height + s->hitboxOffset.y + o->z
                    };

                    // the visual center is offset from the pivot before rotation
                    float localOffsetX = 0.0f;
                    float localOffsetY = -wpnTexHeight / 2.0f;

                    // apply rotation to this offset
                    float angleRad = s->rotationAngle * DEG2RAD;
                    float rotatedOffsetX = localOffsetX * std::cos(angleRad) - localOffsetY * std::sin(angleRad);
                    float rotatedOffsetY = localOffsetX * std::sin(angleRad) + localOffsetY * std::cos(angleRad);

                    // calculate the actual visual center after rotation
                    Vector2 weaponVisualCenter = {
                        rotationPivot.x + rotatedOffsetX,
                        rotationPivot.y + rotatedOffsetY
                    };

                    // place projectile along the aim direction from the visual center
                    float arrowDistance = 8.0f;
                    Vector2 projectileSpawnPos = {
                        weaponVisualCenter.x + aimDirection.x * arrowDistance,
                        weaponVisualCenter.y + aimDirection.y * arrowDistance
                    };

                    Rectangle sRect = {
                        projectileSpawnPos.x - 4.0f,
                        projectileSpawnPos.y - 3.0f,
                        8.0f,
                        2.0f
                    };
                    auto projectile = game.createSprite(data.projectileKey, sRect);
                    projectile->setTextures({ data.projectileKey, data.projectileKey });
                    projectile->addBehavior(std::make_unique<ProjectileBehavior>(
                        game, projectile, nullptr, false, aimDirection,
                        data.projectileTrailEmitterKey, data.projectileImpactEmitterKey
                    ));
                    projectile->canHurtPlayer = false;
                    projectile->canHurtEnemies = true;
                    projectile->speed = 80.0f;
                    projectile->damage = data.damage;
                    projectile->rotationAngle = std::atan2(aimDirection.y, aimDirection.x) * RAD2DEG + 90.0f;
                    game.playSound(data.soundKey);
                    lifetime = 0.0f;
                    o->unlockAnimState();
                    isNotched = false;
                    game.eventManager.pushEvent(UNLOCK_PLAYER_MOVEMENT);
                }
            }
            break;
        }
        default:
            break;
        }
    }
}

void WeaponBehavior::draw() 
{
    if (!isNotched)
        return;
    auto s = self.lock();
    auto o = owner.lock();
    if (!s || !o)
        return;

    // draw the projectile at the weapon's position
    const auto& projectileTextures = game.loader.getTextures(data.projectileKey);
    if (!projectileTextures.empty()) {
        const Texture2D& projectileTex = projectileTextures[0];
        const auto& weaponTextures = s->frames[s->currentAnimState];
        if (weaponTextures.empty())
            return;

        const Texture2D& weaponTex = weaponTextures[s->currentFrame];
        float wpnTexHeight = static_cast<float>(weaponTex.height);

        // calculate the rotation pivot (center-bottom of weapon texture)
        Vector2 rotationPivot = {
            s->position.x + s->rect.width / 2.0f + s->hitboxOffset.x,
            s->position.y + s->rect.height + s->hitboxOffset.y + o->z
        };

        // the visual center is offset from the pivot before rotation
        // offset from center-bottom to center is (0, -height/2)
        float localOffsetX = 0.0f;
        float localOffsetY = -wpnTexHeight / 2.0f;

        // apply rotation to this offset
        float angleRad = s->rotationAngle * DEG2RAD;
        float rotatedOffsetX = localOffsetX * std::cos(angleRad) - localOffsetY * std::sin(angleRad);
        float rotatedOffsetY = localOffsetX * std::sin(angleRad) + localOffsetY * std::cos(angleRad);

        // calculate the actual visual center after rotation
        Vector2 weaponVisualCenter = {
            rotationPivot.x + rotatedOffsetX,
            rotationPivot.y + rotatedOffsetY
        };

        // place arrow along the aim direction from the visual center
        float arrowDistance = 8.0f;
        Vector2 arrowPos = {
            weaponVisualCenter.x + aimDirection.x * arrowDistance,
            weaponVisualCenter.y + aimDirection.y * arrowDistance
        };

        Rectangle source = { 0.0f, 0.0f, static_cast<float>(projectileTex.width), static_cast<float>(projectileTex.height) };
        Rectangle dest = {
            arrowPos.x,
            arrowPos.y,
            static_cast<float>(projectileTex.width),
            static_cast<float>(projectileTex.height)
        };
        Vector2 origin = { static_cast<float>(projectileTex.width) / 2.0f, static_cast<float>(projectileTex.height) / 2.0f };

        DrawTexturePro(projectileTex, source, dest, origin, s->rotationAngle + 90.0f, WHITE);
    }

    // draw aim line with moving dashes
    Vector2 ownerCenter = GetRectCenter(o->rect);
    ownerCenter.y += o->z;  // account for potential jumping

    float lineLength = 200.0f;

    // animated pulsing effect
    float pulse = (std::sin(timer * 5.0f) + 1.0f) * 0.5f;  // 0.0 to 1.0
    float alpha = 0.1f + pulse * 0.5f;

    // dash parameters
    float dashLength = 12.0f;
    float gapLength = 2.0f;
    float segmentLength = dashLength + gapLength;

    // animate the dashes moving forward
    float animationSpeed = 20.0f;
    float offset = fmod(timer * animationSpeed, segmentLength);

    // calculate number of segments needed
    float totalLength = lineLength;
    int numSegments = (int)(totalLength / segmentLength) + 2;

    // draw each dash
    for (int i = 0; i < numSegments; i++)
    {
        float startDist = i * segmentLength + offset;
        float endDist = startDist + dashLength;

        // skip if segment is completely before or after the line
        if (endDist < 0.0f || startDist > totalLength)
            continue;

        // clamp to line bounds
        startDist = fmax(startDist, 0.0f);
        endDist = fmin(endDist, totalLength);

        Vector2 segStart = {
            ownerCenter.x + aimDirection.x * startDist,
            ownerCenter.y + aimDirection.y * startDist
        };
        Vector2 segEnd = {
            ownerCenter.x + aimDirection.x * endDist,
            ownerCenter.y + aimDirection.y * endDist
        };

        DrawLineEx(segStart, segEnd, 2.0f, Fade(GRAY, alpha));
    }
}

void WeaponBehavior::reset()
{
    Behavior::reset();
    lifetime = originalLifetime;
    if (lifetime == -1.0f)
        lifetime = std::numeric_limits<float>::infinity();
    switchedOn = false;
    shaken = false;
}
