#include "Sprite.h"
#include "raymath.h"
#include <iostream>
#include "Game.h"
#include "Controls.h"
#include "Utils.h"


Sprite::Sprite(Game& game, float x, float y, float w, float h, const std::string& spriteName)
    : game(game),
    rect{ x, y, w, h },
    hurtbox{ x, y, w, h},
    acc{ 0.0f, 0.0f },
    vel{ 0.0f, 0.0f },
    position{ (float)x, (float)y },
    spriteName{ spriteName },
    behaviors{},
    health{ 10 }, // default values
    maxHealth{ 10 }
{
    frames["default"] = { game.loader.fallbackTexture };
}

Sprite::~Sprite() {
    // TODO: just for debugging
    TraceLog(LOG_INFO, "Sprite destroyed: %s at %p", spriteName.c_str(), this);
}

void Sprite::setTextures(std::initializer_list<std::string> keys) {
    for (const auto& key : keys) {
        const auto& textures = game.loader.getTextures(key);
        if (textures.empty()) {
            TraceLog(LOG_ERROR, "Missing texture for key: %s", key.c_str());
        }
        frames[key] = textures;
    }
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
    if (behaviors.empty()) return;
    try {
        for (auto& behavior : behaviors) {
            if (!behavior) {
                TraceLog(LOG_WARNING, "Null behavior in %s", spriteName.c_str());
                continue;
            }
            behavior->update(deltaTime);
        }
    }
    catch (const std::exception& e) {
        TraceLog(LOG_FATAL, "Exception in executeBehavior for %s: %s", spriteName.c_str(), e.what());
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
    if (markedForDeletion) return;
    // applies laws of motion according to the acceleration that was
    // set prior to this step (either by player input, Cutscene commands,
    // or a Behavior

    // prevent faster diagonal movement by capping the length to 1
    if (Vector2Length(acc) > 0.0f) acc = Vector2Normalize(acc);

    vel = Vector2Add(vel, Vector2Scale(acc, speed * deltaTime));
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
    // enemies that are already dead can't hurt the player any more
    if (health < 1) {
        canHurtPlayer = false;
    }
}

void Sprite::draw() {
    if (!visible || markedForDeletion) return;

    const std::string& key = (frames.count(currentTextureKey) > 0) ? currentTextureKey : "default";
    auto& textures = frames[key];

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
    if (iFrameTimer > 0.0f && !dying) {
        bool flicker = ((int)(iFrameTimer * 10) % 2) == 0;
        tint = flicker ? Fade(WHITE, 0.5f) : WHITE;
    }
    // draw the texture (change the tint if the sprite has been hit)
    // also rotate around the bottom center
    // and apply a shader, if set
    if (activeShader) {
        int timeLoc = GetShaderLocation(*activeShader->shader, "time");
        int flipXLoc = GetShaderLocation(*activeShader->shader, "flipX");
        int durationLoc = GetShaderLocation(*activeShader->shader, "duration");
        SetShaderValue(*activeShader->shader, timeLoc, &activeShader->time, SHADER_UNIFORM_FLOAT);
        SetShaderValue(*activeShader->shader, flipXLoc, &activeShader->flipX, SHADER_UNIFORM_INT);
        SetShaderValue(*activeShader->shader, durationLoc, &activeShader->duration, SHADER_UNIFORM_FLOAT);
        BeginShaderMode(*activeShader->shader);
    }
    DrawTexturePro(texture, source, dest, origin, rotationAngle, tint);
    if (activeShader) EndShaderMode();
}

