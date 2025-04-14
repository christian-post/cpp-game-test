#include "Sprite.h"
#include "raymath.h"
#include <iostream>
#include "Game.h"


Sprite::Sprite(Game& game, float x, float y, float w, float h, const std::string& spriteName)
    : game(game),
    rect{ x, y, w, h },
    hurtbox{ x, y, w, h},
    acc{ 0.0f, 0.0f },
    vel{ 0.0f, 0.0f },
    position{ (float)x, (float)y },
    spriteName{ spriteName }, // is this even used?
    behaviors{},
    health{ 10 }, // default values
    maxHealth{ 10 }
{
    frames["default"] = { game.loader.fallbackTexture };
}

void Sprite::setTextures(std::initializer_list<std::string> keys) {
    for (const auto& key : keys) {
        frames[key] = game.loader.getTextures(key);
    }
    // Set currentTextureKey to the first key in the list (if any)
    // this ensures that something replaces the default texture (red rectangle) from the start 
    if (keys.begin() != keys.end()) {
        currentTextureKey = *keys.begin();
    }
}

void Sprite::animate(float deltaTime) {
    if (!doesAnimate) return;

    elapsedTime += deltaTime;
    if (elapsedTime >= frameTime && !frames.empty()) {
        elapsedTime = 0.0f;
        currentFrame = (currentFrame + 1) % frames.size();
    }
    // latch the last direction (used for animation)
    if (vel.x == 0.0f && vel.y == 0.0f) {
        currentTextureKey = spriteName + "_idle";
    }
    else {
        currentTextureKey = spriteName + "_run";
        if (vel.x > 0.0f) {
            lastDirection = RIGHT; // right
        }
        else if (vel.x < 0.0f) {
            lastDirection = LEFT; // left
        }
    }
}

void Sprite::setHurtbox(float x, float y, float width, float height, bool center) {
    if (x != -1.0f) hurtbox.x = x;
    if (y != -1.0f) hurtbox.y = y;
    if (width != -1.0f) hurtbox.width = width;
    if (height != -1.0f) hurtbox.height = height;
    // center on hitbox
    if (center) {
        float w = (width != -1.0f) ? width : hurtbox.width;
        float h = (height != -1.0f) ? height : hurtbox.height;
        hurtbox.x = rect.x + (rect.width - w) / 2;
        hurtbox.y = rect.y + (rect.height - h) / 2;
    }
    // move by offset
    hurtbox.x += hurtboxOffset.x;
    hurtbox.y += hurtboxOffset.y;
}

void Sprite::getControls() {
    // calculate acceleration vector from player input
    acc.x = static_cast<float>((bool)(game.buttonsDown & CONTROL_RIGHT) - (bool)(game.buttonsDown & CONTROL_LEFT));
    acc.y = static_cast<float>((bool)(game.buttonsDown & CONTROL_DOWN) - (bool)(game.buttonsDown & CONTROL_UP));
}

void Sprite::executeBehavior(float deltaTime) {
    // runs update() on the behaviors in the order that they were given
    // TODO: behavior priority system?
    for (auto& behavior : behaviors) {
        behavior->update(deltaTime);
    }
}

void Sprite::moveTo(float x, float y) {
    // moves the sprite instantly, without applying physics
    // makes sure all the rects are placed accordingly
    position = { x, y };
    rect.x = position.x;
    rect.y = position.y;
    // center the hurtbox
    hurtbox.x = rect.x + (rect.width - hurtbox.width) / 2 + hurtboxOffset.x;
    hurtbox.y = rect.y + (rect.height - hurtbox.height) / 2 + hurtboxOffset.y;
}

void Sprite::update(float deltaTime) {
    // applies laws of motion according to the acceleration that was
    // set prior to this step (either by player input, Cutscene commands,
    // or a Behavior

    // prevent faster diagonal movement by capping the length to 1
    if (Vector2Length(acc) > 0.0f) acc = Vector2Normalize(acc);

    vel = Vector2Add(vel, Vector2Scale(acc, speed * deltaTime));
    //Log(vel);
    vel = Vector2Scale(vel, friction);

    // stop if vel is below a threshold to prevent jitter
    if (vel.x * vel.x + vel.y * vel.y < 2.5e-3f) {
        vel = { 0.0f, 0.0f };
    }

    position = Vector2Add(position, vel);

    // reset acceleration
    acc = { 0.0f, 0.0f };

    // damage handling
    if (iFrameTimer > 0.0f) {
        // sprite is invincible
        iFrameTimer -= deltaTime;
    }
}

void Sprite::draw() {
    const std::string& key = (frames.count(currentTextureKey) > 0) ? currentTextureKey : "default";

    auto& textures = frames[key];
    //if (textures.empty()) return;

    if (currentFrame >= textures.size()) {
        DrawRectangleRec(rect, BLUE);
        return;
    }

    Texture2D& texture = textures[currentFrame];

    // Define source and destination rectangles
    Rectangle source = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };

    Rectangle dest = {
        rect.x + rect.width / 2.0f,  // X: center of the sprite
        rect.y + rect.height,        // Y: bottom of the sprite
        source.width, source.height
    };

    Vector2 origin = { source.width / 2.0f, source.height };

    // Flip horizontally if lastDirection is LEFT
    if (lastDirection == LEFT) {
        source.width = -source.width;
    }

    Color tint = WHITE;
    if (iFrameTimer > 0.0f) {
        bool flicker = ((int)(iFrameTimer * 10) % 2) == 0;
        tint = flicker ? Fade(WHITE, 0.5f) : WHITE;
    }
    // draw the texture (change the tint if the sprite has been hit)
    // also rotate around the bottom center
    DrawTexturePro(texture, source, dest, origin, rotationAngle, tint);
}

