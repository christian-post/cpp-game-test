#pragma once
#include "Sprite.h"
#include <string>
#include "TextBox.h"
#include <memory>
#include "Game.h"

class Game;


class Command {
	// base class for all cutscene commands
public:
	virtual ~Command() = default;
	virtual void update(float deltaTime) = 0;
    virtual void draw() {};
	bool isDone() const { return done; };
    std::string name = "base";

    bool isPersistent() const { return persistent; }

protected:
	bool started = false;
    bool persistent = false;
	bool done = false;
};


class Command_Wait : public Command {
    // just waits for a given amount of time
public:
    Command_Wait(float duration) : duration{ duration } {
        started = true;
        name = "wait"; // TODO. just for debugging
    };

    void update(float deltaTime) override {
        timer += deltaTime;
        if (timer >= duration) {
            done = true;
            return;
        }
    }

private:
    float duration;
    float timer = 0.0f;
};


class Command_MoveTo : public Command {
	// moves a sprite to a specified position in a given time
public:
	Command_MoveTo(Sprite& target, float posX, float posY, float duration) :
		target{ target}, finalPosX{ posX }, finalPosY{ posY }, duration{ duration } {
        name = "MoveTo";
    };

    void update(float deltaTime) override {
        if (!started) {
            startX = target.position.x;
            startY = target.position.y;
            started = true;
        }

        timer += deltaTime;
        if (timer >= duration) {
            target.position.x = finalPosX;
            target.position.y = finalPosY;
            target.vel = { 0.0f, 0.0f };
            done = true;
            return;
        }

        float tPct = timer / duration;
        float newX = startX + (finalPosX - startX) * tPct;
        float newY = startY + (finalPosY - startY) * tPct;

        // Calculate velocity based on position delta (currently only used for animation state)
        target.vel.x = (newX - target.position.x) / deltaTime;
        target.vel.y = (newY - target.position.y) / deltaTime;

        // also set the position (don't rely on physics to be accurate)
        target.position.x = newX;
        target.position.y = newY;
        target.rect.x = newX;
        target.rect.y = newY;
        
        // adjust the rect
        // TODO: only temporary until the NPCs collision detection is sorted out
        target.rect.x = target.position.x;
        target.rect.y = target.position.y;
    }

private:
	Sprite& target;
	float startX = 0.0f; 
	float startY = 0.0f;
	float finalPosX, finalPosY;
	float duration;
	float timer = 0.0f;
};

class Command_Look : public Command {
    // the target Sprite will turn in this direction once
public:
    Command_Look(Sprite& target, direction dir) : target{ target }, dir{ dir } {
        name = "Look";
    };

    void update(float deltaTime) {
        if (!started) {
            target.lastDirection = dir;
            started = true;
            done = true;
        }
    }

private:
    Sprite& target;
    direction dir;
};


class Command_LookTowards : public Command {
    // the target Sprite will turn, depending on the position
    // relative to the other sprite
public:
    Command_LookTowards(Sprite& target, Sprite& other) : target{ target }, other{ other } {
        name = "LookTowards";
    };

    void update(float deltaTime) {
        if (!started) {
            if (other.position.x > target.position.x) {
                target.lastDirection = RIGHT;
            }
            else {
                target.lastDirection = LEFT;
            }
            started = true;
            done = true;
        }
    }

private:
    Sprite& target;
    Sprite& other;
};


class Command_Textbox : public Command {
public:
    Command_Textbox(Game& game, std::string_view text)
        : textToDisplay(text)
    {
        float w = game.gameScreenWidth * 0.9f;
        float h = game.getSetting("textboxHeight");
        float x = game.gameScreenWidth * 0.05f;
        // change Y position depending on where the player is
        float y;
        if (game.getPlayer()->position.y > game.gameScreenHeight * 0.5f) {
            y = 24.0f;
        }
        else {
            y = game.gameScreenHeight - h - game.gameScreenHeight * 0.05f;
        }

        textbox = new TextBox(game, x, y, w, h, 10);

        name = "TextBox";
    }

    ~Command_Textbox() {
        delete textbox;
    }

    void update(float deltaTime) override {
        if (!started) {
            textbox->setTextContent(textToDisplay);
            started = true;
        }

        textbox->update(deltaTime);
        done = textbox->isFinished();
    }

    void draw() override {
        textbox->draw();
    }

private:
    TextBox* textbox = nullptr;
    std::string_view textToDisplay;
};


class Command_Callback : public Command {
    // queue a generic callback
public:
    Command_Callback(std::function<void()> callback) : callback{ callback } {
        name = "Callback";
    }

    void update(float deltaTime) override {
        if (!started) {
            callback();
            started = true;
            done = true;
        }
    }

private:
    std::function<void()> callback;
};


class Command_Letterbox : public Command {
public:
    Command_Letterbox(float screenWidth, float screenHeight, float duration)
        : screenWidth(screenWidth), screenHeight(screenHeight), speed(24.0f / duration) {
        started = true;
        name = "Letterbox";
        persistent = true;
    }

    void update(float deltaTime) override {
        if (barHeight < 24.0f) {
            barHeight = std::min(24.0f, barHeight + deltaTime * speed);
        }
        else {
            done = true;
        }
    }

    void draw() override {
        if (barHeight > 0.0f) {
            DrawRectangle(0, 0, int(screenWidth), int(barHeight), BLACK);
            DrawRectangle(0, int(screenHeight - barHeight), int(screenWidth), int(barHeight), BLACK);
        }
    }

private:
    float screenWidth;
    float screenHeight;
    float barHeight = 0.0f;
    float speed;
};
