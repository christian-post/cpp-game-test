#include "Commands.h"
#include "Game.h"
#include "TextBox.h"
#include "raylib.h"

Command_Wait::Command_Wait(float duration) : duration(duration) {
    started = true; name = "wait";
}

void Command_Wait::update(float deltaTime) {
    timer += deltaTime;
    if (timer >= duration) done = true;
}

Command_MoveTo::Command_MoveTo(Sprite& target, float posX, float posY, float duration)
    : target(target), finalPosX(posX), finalPosY(posY), duration(duration) {
    name = "MoveTo";
}

void Command_MoveTo::update(float deltaTime) {
    if (!started) {
        startX = target.position.x; startY = target.position.y;
        started = true;
    }
    timer += deltaTime;
    if (timer >= duration) {
        target.position.x = finalPosX; target.position.y = finalPosY;
        target.vel = { 0.0f, 0.0f }; done = true; return;
    }
    float tPct = timer / duration;
    float newX = startX + (finalPosX - startX) * tPct;
    float newY = startY + (finalPosY - startY) * tPct;
    target.vel.x = (newX - target.position.x) / deltaTime;
    target.vel.y = (newY - target.position.y) / deltaTime;
    target.position.x = newX; target.position.y = newY;
    target.rect.x = newX; target.rect.y = newY;
}

Command_Look::Command_Look(Sprite& target, direction dir) : target(target), dir(dir) {
    name = "Look";
}

void Command_Look::update(float deltaTime) {
    if (!started) { target.lastDirection = dir; started = true; done = true; }
}

Command_LookTowards::Command_LookTowards(Sprite& target, Sprite& other)
    : target(target), other(other) {
    name = "LookTowards";
}

void Command_LookTowards::update(float deltaTime) {
    if (!started) {
        target.lastDirection = (other.position.x > target.position.x) ? RIGHT : LEFT;
        started = true; done = true;
    }
}

Command_Textbox::Command_Textbox(Game& game, std::string text, std::string voice) : textToDisplay{ text }, voice{ voice } {
    float w = game.gameScreenWidth * 0.9f;
    float x = game.gameScreenWidth * 0.05f;
    float h = game.getSetting("textboxHeight").get<float>();
    float y = (game.getPlayer()->position.y > game.gameScreenHeight * 0.5f)
        ? game.getSetting("HudHeight").get<float>()
        : game.gameScreenHeight - h - game.gameScreenHeight * 0.05f;
    textbox = new TextBox(game, x, y, w, h, 10, voice);
    textbox->setVoicePitch(false);
    name = "TextBox";
}

Command_Textbox::~Command_Textbox() { delete textbox; }

void Command_Textbox::update(float deltaTime) {
    if (!started) { textbox->setTextContent(textToDisplay); started = true; }
    textbox->update(deltaTime);
    done = textbox->isFinished();
}

void Command_Textbox::draw() { textbox->draw(); }

Command_Callback::Command_Callback(std::function<void()> callback) : callback(callback) {
    name = "Callback";
}

void Command_Callback::update(float deltaTime) {
    if (!started) { callback(); started = true; done = true; }
}

Command_Letterbox::Command_Letterbox(float screenWidth, float screenHeight, float duration)
    : screenWidth(screenWidth), screenHeight(screenHeight), speed(24.0f / duration) {
    started = true; name = "Letterbox"; persistent = true;
}

void Command_Letterbox::update(float deltaTime) {
    if (barHeight < 24.0f) barHeight = std::min(24.0f, barHeight + deltaTime * speed);
    else done = true;
}

void Command_Letterbox::draw() {
    if (barHeight > 0.0f) {
        DrawRectangle(0, 0, (int)screenWidth, (int)barHeight, BLACK);
        DrawRectangle(0, (int)(screenHeight - barHeight), (int)screenWidth, (int)barHeight, BLACK);
    }
}
