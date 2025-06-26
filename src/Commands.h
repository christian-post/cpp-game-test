#pragma once
#include "Sprite.h"
#include <string>
#include <memory>
#include <functional>

class Game;
class TextBox;

class Command {
public:
    virtual ~Command() = default;
    virtual void update(float deltaTime) = 0;
    virtual void draw() {};
    bool isDone() const { return done; }
    std::string name = "base";
    bool isPersistent() const { return persistent; }

protected:
    bool started = false;
    bool persistent = false;
    bool done = false;
};

class Command_Wait : public Command {
public:
    Command_Wait(float duration);
    void update(float deltaTime) override;

private:
    float duration;
    float timer = 0.0f;
};

class Command_MoveTo : public Command {
public:
    Command_MoveTo(Sprite& target, float posX, float posY, float duration);
    void update(float deltaTime) override;

private:
    Sprite& target;
    float startX = 0.0f, startY = 0.0f, finalPosX, finalPosY, duration, timer = 0.0f;
};

class Command_Look : public Command {
public:
    Command_Look(Sprite& target, direction dir);
    void update(float deltaTime) override;

private:
    Sprite& target;
    direction dir;
};

class Command_LookTowards : public Command {
public:
    Command_LookTowards(Sprite& target, Sprite& other);
    void update(float deltaTime) override;

private:
    Sprite& target;
    Sprite& other;
};

class Command_Textbox : public Command {
public:
    Command_Textbox(Game& game, std::string text, std::string voice = "tone", bool pitch = false);
    ~Command_Textbox();
    void update(float deltaTime) override;
    void draw() override;

private:
    TextBox* textbox = nullptr;
    std::string textToDisplay;
    std::string voice;
    static inline float textboxCooldownTimer = 0.0f;
    static inline bool textboxCooldown = false;

public:
    static void updateCooldown(float deltaTime); // advance the timer when this Command is inactive
    static bool isTextboxCooldown() { return textboxCooldown; }
};

class Command_Callback : public Command {
public:
    Command_Callback(std::function<void()> callback);
    void update(float deltaTime) override;

private:
    std::function<void()> callback;
};

class Command_Letterbox : public Command {
public:
    Command_Letterbox(float screenWidth, float screenHeight, float duration);
    void update(float deltaTime) override;
    void draw() override;

private:
    float screenWidth, screenHeight, barHeight = 0.0f, speed;
};

class Command_CameraPan : public Command {
public:
    Command_CameraPan(Game& game, float targetX, float targetY, float duration);
    void update(float deltaTime) override;

private:
    Game& game;
    float startX = 0.0f;
    float startY = 0.0f;
    float targetX, targetY;
    float duration;
    float elapsed = 0.0f;
};

