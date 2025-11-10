#pragma once
#include "Sprite.h"
#include <string>
#include <memory>
#include <functional>

class Game;
class TextBox;

class Command {
    // base class for cutscene commands
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
    // does nothing for a given amount of time.
    // blocks all following commands
public:
    Command_Wait(float duration);
    void update(float deltaTime) override;

private:
    float duration;
    float timer = 0.0f;
};

class Command_MoveTo : public Command {
    // > makes the target sprite walk to a given destination.
    // > plays the RUN state animation while the timer is > 0.
    // > Use duration = 0.0f to teleport a sprite instantly
public:
    Command_MoveTo(Sprite& target, float posX, float posY, float duration);
    void update(float deltaTime) override;

private:
    Sprite& target;
    float startX = 0.0f, startY = 0.0f, finalPosX, finalPosY, duration, timer = 0.0f;
};

class Command_Look : public Command {
    // target sprite looks in a fixed direction (LEFT or RIGHT)
public:
    Command_Look(Sprite& target, direction dir);
    void update(float deltaTime) override;

private:
    Sprite& target;
    direction dir;
};

class Command_LookTowards : public Command {
    // direction depends on the difference between two sprites' x positions
public:
    Command_LookTowards(Sprite& target, Sprite& other);
    void update(float deltaTime) override;

private:
    Sprite& target;
    Sprite& other;
};

class Command_Textbox : public Command {
    // displays some text
    // the "pitch" argument controls whether the tone should be slightly shifted based on the length of each word
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
    // executes arbitrary code during a cutscene
public:
    Command_Callback(std::function<void()> callback);
    void update(float deltaTime) override;

private:
    std::function<void()> callback;
};

class Command_Letterbox : public Command {
    // displays black bars on the top and bottom of the screen
    // the "duration" argument controls how long it takes the bars to move to their final position
    // TODO: make a function to fade out the letterbox
public:
    Command_Letterbox(float screenWidth, float screenHeight, float duration);
    void update(float deltaTime) override;
    void draw() override;

private:
    float screenWidth, screenHeight, barHeight = 0.0f, speed;
};

class Command_CameraPan : public Command {
    // moves the camera towards a given position
    // TODO: ease in/out
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

