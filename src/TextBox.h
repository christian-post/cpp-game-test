#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class Game;

class TextBox {
public:
    Game& game;
    TextBox(Game& game, float x, float y, float width, float height, int fontSize);
    void update(float dt);
    void draw();

    void setTextContent(std::string_view text);
    void formatText();
    bool isFinished() const { return finished; }

private:
    float x, y, width, height;
    std::string_view textContent;
    std::string formattedText;  // Stores text with line breaks
    int fontSize;

    size_t currentStrIndex = 0;
    int currentLine = 0;
    size_t currentPageStartIndex = 0;
    bool pageDone = false;
    bool finished = false;
    float timer = 0.0f;
    float textSpeed = 0.005f;  // how often a new char appears, in ms
};
