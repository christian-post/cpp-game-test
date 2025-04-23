#include "TextBox.h"
#include <iostream>
#include "Game.h"
#include "Utils.h"
#include "Controls.h"

TextBox::TextBox(Game& game, float x, float y, float width, float height, int fontSize)
    : game(game), 
    x{ x }, 
    y{ y }, 
    width{ width }, 
    height{ height }, 
    fontSize{ fontSize }, 
    textContent{ "" } {
}

void TextBox::formatText() {
    formattedText.clear();
    std::string_view text = textContent.substr(currentPageStartIndex);  // Avoid copying

    size_t start = 0;
    size_t spacePos = 0;  // position of the next whitespace
    std::string line;

    while ((spacePos = text.find(' ', start)) != std::string_view::npos) {
        std::string_view word = text.substr(start, spacePos - start);
        std::string testLine = line.empty() ? std::string(word) : line + " " + std::string(word);

        if (MeasureText(testLine.c_str(), fontSize) > int(width) - 10) {
            formattedText += line + "\n";

            currentLine++;

            // check if the next line would exceed the box
            Vector2 size = MeasureTextEx(GetFontDefault(), formattedText.c_str(), float(fontSize), 2.0f);
            // measure one line
            float lineSize = size.y / (currentLine + 1);
            // test if another line would exceed the box
            // TODO: somehow this is inconsistent
            if (lineSize * (currentLine + 1) + lineSize > height) {
                pageDone = true;
                currentPageStartIndex += start;
                currentLine = 0;
                return;
            }
            line = std::string(word);  // Start new line
        }
        else {
            line = testLine;
        }
        start = spacePos + 1;  // Move to next word
    }

    // Add the last word/line
    if (start < text.size()) {
        if (!line.empty()) line += " ";
        line += std::string(text.substr(start));
    }
    formattedText += line;
}


void TextBox::setTextContent(std::string_view text) {
    textContent = text;
    formatText();
}

void TextBox::update(float dt) {
    // show more than one character if text speed is faster than the frame rate
    int charAtATime = 1;
    if (textSpeed < dt) {
        charAtATime = int(dt / textSpeed);
    }

    timer += dt;
    if (timer >= textSpeed) {
        timer = 0.0f;
        currentStrIndex += charAtATime;
    }
    // show all text if user presses a button
    if (game.buttonsPressed & CONTROL_ACTION1) {
        if (currentStrIndex < formattedText.length() - 1) {
            currentStrIndex = formattedText.length() - 1;
        }
        else {
            if (pageDone) {
                currentStrIndex = 0;
                pageDone = false;
                formatText();
            }
            else {
                Log("finished");
                finished = true;
            }
        }
    }
}

void TextBox::draw() {
    DrawRectangle(int(x), int(y), int(width), int(height), BLACK);
    DrawText(formattedText.substr(0, currentStrIndex).data(), int(x) + 5, int(y) + 5, fontSize, WHITE);
}

