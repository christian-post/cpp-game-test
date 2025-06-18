#include "MapUI.h"
#include "Game.h"
#include "Controls.h"
#include "raylib.h"
#include "Utils.h"

MapUI::MapUI(Game& game, const std::string& name) : Scene(game, name) {}

void MapUI::startup() {
    x = float(game.gameScreenWidth); // start the map screen on the right
    topY = game.getSetting("HudHeight");
    y = topY;
    width = game.gameScreenWidth;
    height = game.gameScreenHeight - game.getSetting("HudHeight");
    // set the sliding speed so that it takes "slideDuration" seconds to expand the inventory
    speed = height / slideDuration;
    state = SLIDING_LEFT;
}

void MapUI::update(float deltaTime) {
    blinkTimer += deltaTime;
    if (blinkTimer >= blinkSpeed) {
        cursorOn = !cursorOn;
        blinkTimer -= blinkSpeed;
    }
    switch (state) {
    case OPENING:
        if (y > topY) {
            y = std::max(topY, y - deltaTime * speed);
        }
        else {
            state = OPENED;
        }
        break;
    case CLOSING:
        if (y < game.gameScreenHeight) {
            y = std::min(static_cast<float>(game.gameScreenHeight), y + deltaTime * speed);
        }
        else {
            game.eventManager.pushEvent("InventoryDone");
            game.stopScene("MapUI");
        }
        break;
    case SLIDING_LEFT:
        if (x > 0.0f) {
            x = std::max(0.0f, x - deltaTime * speed);
        } 
        else {
            state = OPENED;
            blinkTimer = 0.0f;
        }
        break;
    case SLIDING_RIGHT:
        if (x < static_cast<float>(game.gameScreenWidth)) {
            x = std::min(static_cast<float>(game.gameScreenWidth), x + deltaTime * speed);
        }
        else {
            state = NONE;
            game.pauseScene("MapUI");
        }
        break;
    case OPENED:
        if (game.buttonsPressed & CONTROL_CONFIRM) {
            state = CLOSING;
            game.playSound("menuClose");
        }
        if (game.buttonsPressed & CONTROL_ACTIONL) {
            state = SLIDING_RIGHT;
            game.playSound("menuOpen");
            game.resumeScene("InventoryUI");
        }
        break;
    }
}

void MapUI::draw() {
    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);

    // draw the room layout
    // TODO: hard coding the values for now
    const int spacing = 4;
    const int border = 24;
    auto [cols, rows] = game.currentDungeon->getSize();
    size_t currentRoomIndex = game.currentDungeon->getCurrentRoomIndex();
    const int cellWidth = (width - 2 * border - (cols - 1) * spacing) / cols;
    const int cellHeight = (height - 2 * border - (rows - 1) * spacing) / rows;

    auto& minimaps = game.currentDungeon->minimapTextures;
    for (int i = 0; i < cols * rows; ++i) {
        int col = i % cols;
        int row = i / cols;
        int cellX = x + border + col * (cellWidth + spacing);
        int cellY = y + border + row * (cellHeight + spacing);
        Color color = DARKGRAY;
        DrawRectangle(cellX, cellY, cellWidth, cellHeight, color);

        if (i < minimaps.size() && game.currentDungeon->hasVisited(i)) {
            const auto& tex = minimaps[i].texture;
            Rectangle src = { 0, 0, (float)tex.width, -(float)tex.height };
            Rectangle dst = { (float)cellX, (float)cellY, (float)cellWidth, (float)cellHeight };
            DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);
            if (i == currentRoomIndex && cursorOn && state == OPENED) {
                // draw a player as a blinking circle
                const Vector2& pos = game.getPlayer()->position;
                auto [roomW, roomH] = game.currentDungeon->getRoomSize(currentRoomIndex);
                float u = pos.x / (float)roomW;
                float v = pos.y / (float)roomH;
                float px = cellX + u * cellWidth;
                float py = cellY + v * cellHeight;
                //Color c = { 114, 214, 206, 255 };
                //DrawCircle((int)px, (int)py, 3.0f, c);
                const auto& tex = game.loader.getTextures("knight_map_mini")[0];
                DrawTexture(tex, (int)px, (int)py, WHITE);
            }
        }
    }

    // help texts
    if (state == OPENED) {
        int fontSize = 6;
        const char* helpText = nullptr;
        const char* textLeft = nullptr;
        const char* textRight = nullptr;
        if (WasGamepadUsedLast()) {
            textLeft = "<< LB";
            textRight = "RB >>";
        }
        else {
            textLeft = "<< N";
            textRight = "M >>";
        }
        uint32_t helpTextY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 8;
        uint32_t txtR = int(x) + int(game.gameScreenWidth) - MeasureText(textRight, fontSize) - 4;
        uint32_t txtL = int(x) + 4;
        DrawText(textRight, txtR, helpTextY, fontSize, LIGHTGRAY);
        DrawText(textLeft, txtL, helpTextY, fontSize, LIGHTGRAY);
    }
}

void MapUI::end() {
    game.eventManager.pushEvent("setMusicVolume", 1.0f);
}