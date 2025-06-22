#include "TextBox.h"
#include <iostream>
#include "Game.h"
#include "Utils.h"
#include "Controls.h"

TextBox::TextBox(Game& game, float x, float y, float width, float height, int fontSize, std::string voice)
    : game(game), 
    x{ x }, 
    y{ y }, 
    width{ width }, 
    height{ height }, 
    fontSize{ fontSize }, 
    textContent{ "" } ,
    voice{ voice }
{
    textSpeed = game.getSetting("textDelay");
}

void TextBox::formatText() {
    formattedtext.clear();
    std::string_view text = textContent.substr(currentPageStartIndex);  // Avoid copying

    size_t start = 0;
    size_t spacePos = 0;  // position of the next whitespace
    std::string line;

    while ((spacePos = text.find(' ', start)) != std::string_view::npos) {
        std::string_view word = text.substr(start, spacePos - start);
        std::string testLine = line.empty() ? std::string(word) : line + " " + std::string(word);

        if (MeasureText(testLine.c_str(), fontSize) > int(width) - 10) {
            formattedtext += line + "\n";

            currentLine++;

            // check if the next line would exceed the box
            Vector2 size = MeasureTextEx(GetFontDefault(), formattedtext.c_str(), float(fontSize), 2.0f);
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
    formattedtext += line;
}


void TextBox::setTextContent(std::string_view text) {
    textContent = text;
    formatText();
}

void TextBox::update(float deltaTime) {
    // show more than one character if text speed is faster than the frame rate
    size_t charAtATime = 1;
    if (textSpeed < deltaTime) {
        charAtATime = static_cast<size_t>(deltaTime / textSpeed);
    }

    timer += deltaTime;
    if (timer >= textSpeed) {
        timer = 0.0f;
        size_t oldIndex = currentStrIndex;
        currentStrIndex += charAtATime;
        // Play a randomly pitched sound at the beginning of a word
        // TODO: currently unused
        /*for (int i = oldIndex; i < currentStrIndex && i < formattedtext.size(); ++i) {
            if (i == 0 || formattedtext[i - 1] == ' ' || formattedtext[i - 1] == '\n') {
                Sound& s = game.loader.getSound("powerUp4");
                float pitch = getRandomFloat(0.8f, 1.2f);
                SetSoundPitch(s, pitch);
                PlaySound(s);
                break;
            }
        }*/
        // play a pitched sound; the pitch is determined by the word length
        for (size_t i = oldIndex; i < currentStrIndex && i < formattedtext.size(); ++i) {
            if (!game.soundOn)
                break;
            if (i == 0 || formattedtext[i - 1] == ' ' || formattedtext[i - 1] == '\n') {
                Sound& s = game.loader.getSound(voice);
                if (pitchVoice) {
                    // Map word length to pitch (example: longer words = lower pitch)
                    // Find word length
                    int wordLen = 0;
                    while (i + wordLen < formattedtext.size() &&
                        formattedtext[i + wordLen] != ' ' &&
                        formattedtext[i + wordLen] != '\n') {
                        ++wordLen;
                    }
                    float pitch = 1.2f - 0.05f * std::min(wordLen, 10);
                    SetSoundPitch(s, pitch);
                }
                PlaySound(s);
                break;
            }
        }
    }
    // show all text if user presses a button
    if (game.buttonsPressed & CONTROL_ACTION1) {
        if (currentStrIndex < formattedtext.length() - 1) {
            currentStrIndex = formattedtext.length() - 1;
        }
        else {
            if (pageDone) {
                currentStrIndex = 0;
                pageDone = false;
                formatText();
            }
            else {
                TraceLog(LOG_INFO, "text finished");
                finished = true;
            }
        }
    }
}

void TextBox::draw() {
    DrawRectangle(int(x), int(y), int(width), int(height), BLACK);
    DrawText(formattedtext.substr(0, currentStrIndex).data(), int(x) + 5, int(y) + 5, fontSize, WHITE);
}

