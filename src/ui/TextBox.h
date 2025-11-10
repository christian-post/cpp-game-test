#pragma once
#include "raylib.h"
#include <string>
#include <vector>

class Game;

class TextBox {
public:
    Game& game;
    TextBox(Game& game, float x, float y, float width, float height, int fontSize, std::string voice);
    void update(float deltaTime);
    void draw();

    void setTextContent(std::string_view text);
    void formatText();
    bool isFinished() const { return finished; }
    void setVoice(std::string voice) { voice = voice; }
    void setVoicePitch(bool set) { pitchVoice = set; }

private:
    float x, y, width, height;
    std::string_view textContent;
    std::string formattedtext;  // Stores text with line breaks
    int fontSize;

    size_t currentStrIndex = 0;
    int currentLine = 0;
    size_t currentPageStartIndex = 0;
    bool pageDone = false;
    bool finished = false;
    float timer = 0.0f;
    float textSpeed = 0.005f;  // how often a new char appears, in ms
    std::string voice = "tone";  // key for the sound that is played at each word
    bool pitchVoice = true;
    void endPage(size_t index);
};
