#include "TextBox.h"
#include "Game.h"
#include "Utils.h"
#include "Controls.h"
#include <iostream>

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
    textSpeed = game.getSetting<float>("textDelay");
}

void TextBox::endPage(size_t index)
{ 
    pageDone = true;
    currentPageStartIndex += index;
    currentLine = 0;
}

void TextBox::formatText()
{
    formattedtext.clear();
    std::string_view text = textContent.substr(currentPageStartIndex);

    // Handle forced page break
    size_t ffPos = text.find('\f');
    if (ffPos != std::string_view::npos)
        text = text.substr(0, ffPos);

    size_t start = 0;
    size_t spacePos = 0;
    std::string line;

    while ((spacePos = text.find(' ', start)) != std::string_view::npos)
    {
        std::string_view word = text.substr(start, spacePos - start);
        std::string testLine = line.empty() ? std::string(word) : line + " " + std::string(word);

        int lineWidth = hasTokens ? MeasureTextWithSprites(testLine.c_str(), fontSize, game)
            : MeasureText(testLine.c_str(), fontSize);

        if (lineWidth > int(width) - 10)
        {
            formattedtext += line + "\n";
            currentLine++;
            Vector2 size = MeasureTextEx(GetFontDefault(), formattedtext.c_str(), float(fontSize), 2.0f);
            float lineHeight = size.y / (currentLine + 1);
            if (lineHeight * (currentLine + 1) + lineHeight > height)
            {
                endPage(start);
                return;
            }
            line = std::string(word);
        }
        else
        {
            line = testLine;
        }
        start = spacePos + 1;
    }

    // handle last word (same change needed here)
    if (start < text.size())
    {
        std::string_view word = text.substr(start);
        std::string testLine = line.empty() ? std::string(word) : line + " " + std::string(word);

        int lineWidth = hasTokens ? MeasureTextWithSprites(testLine.c_str(), fontSize, game)
            : MeasureText(testLine.c_str(), fontSize);

        if (lineWidth > int(width) - 10)
        {
            formattedtext += line + "\n";
            currentLine++;
            Vector2 size = MeasureTextEx(GetFontDefault(), formattedtext.c_str(), float(fontSize), 2.0f);
            float lineHeight = size.y / (currentLine + 1);
            if (lineHeight * (currentLine + 1) + lineHeight > height)
            {
                endPage(start);
                return;
            }
            line = std::string(word);
        }
        else
        {
            line = testLine;
        }
    }

    formattedtext += line;
    if (ffPos != std::string_view::npos)
        endPage(ffPos + 1);
}

void TextBox::setTextContent(std::string_view text)
{
    textContent = text;
    hasTokens = (text.find("[TEX:") != std::string_view::npos);  // check once
    formatText();
}

void TextBox::update(float deltaTime)
{
    // show more than one character if text speed is faster than the frame rate
    size_t charAtATime = 1;
    if (textSpeed < deltaTime)
        charAtATime = static_cast<size_t>(deltaTime / textSpeed);

    timer += deltaTime;
    if (timer >= textSpeed)
    {
        timer = 0.0f;
        size_t oldIndex = currentStrIndex;
        currentStrIndex += charAtATime;

        // if we're inside a token, skip to the end of it
        if (hasTokens && currentStrIndex < formattedtext.size())
        {
            size_t tokenStart = formattedtext.rfind("[TEX:", currentStrIndex);
            if (tokenStart != std::string::npos && tokenStart < currentStrIndex)
            {
                size_t tokenEnd = formattedtext.find("]", tokenStart);
                if (tokenEnd != std::string::npos && tokenEnd >= currentStrIndex)
                    currentStrIndex = tokenEnd + 1;
            }
        }

        // play a pitched sound; the pitch is determined by the word length
        for (size_t i = oldIndex; i < currentStrIndex && i < formattedtext.size(); ++i)
        {
            if (!game.soundOn)
                break;

            if (i == 0 || formattedtext[i - 1] == ' ' || formattedtext[i - 1] == '\n')
            {
                Sound& s = game.loader.getSound(voice);
                if (pitchVoice)
                {
                    // Map word length to pitch (example: longer words = lower pitch)
                    // Find word length
                    int wordLen = 0;
                    while (i + wordLen < formattedtext.size() &&
                        formattedtext[i + wordLen] != ' ' &&
                        formattedtext[i + wordLen] != '\n')
                    {
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
    if (game.buttonsPressed & CONTROL_ACTION1)
    {
        if (currentStrIndex < formattedtext.length() - 1)
        {
            currentStrIndex = formattedtext.length() - 1;
        }
        else
        {
            if (pageDone)
            {
                currentStrIndex = 0;
                pageDone = false;
                formatText();
            }
            else
            {
                TraceLog(LOG_INFO, "text finished");
                finished = true;
            }
        }
    }
}

void TextBox::draw()
{
    DrawRectangle(int(x), int(y), int(width), int(height), BLACK);

    if (hasTokens)
    {
        DrawTextWithSprites(formattedtext.substr(0, currentStrIndex).data(),
            int(x) + 5, int(y) + 5, fontSize, WHITE, game);
    }
    else
    {
        DrawText(formattedtext.substr(0, currentStrIndex).data(),
            int(x) + 5, int(y) + 5, fontSize, WHITE);
    }
}

