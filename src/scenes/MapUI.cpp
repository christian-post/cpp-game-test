#include "MapUI.h"
#include "Game.h"
#include "Controls.h"
#include "Utils.h"

MapUI::MapUI(Game& game, const std::string& name) : Scene(game, name) 
{}

void MapUI::startup()
{
    x = float(game.gameScreenWidth); // start the map screen on the right
    topY = game.getSetting<float>("HudHeight");
    y = topY;
    // recalculate size values
    width = game.gameScreenWidth;
    height = game.gameScreenHeight - game.getSetting<uint32_t>("HudHeight");
    // set the sliding speed so that it takes [slideDuration] seconds to expand the inventory
    speed = height / slideDuration;
    state = SLIDING_LEFT;

    currentLevel = game.currentDungeon->getCurrentLevel();
}

void MapUI::update(float deltaTime)
{
    blinkTimer += deltaTime;
    if (blinkTimer >= blinkSpeed) {
        cursorOn = !cursorOn;
        blinkTimer -= blinkSpeed;
    }
    // state machine
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
            game.eventManager.pushEvent(INVENTORY_DONE);
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
        if (game.buttonsPressed & CONTROL_UP) {
            currentLevel = (currentLevel + 1) % game.currentDungeon->getNumLevels();
        }
        if (game.buttonsPressed & CONTROL_DOWN) {
            currentLevel = (currentLevel - 1) % game.currentDungeon->getNumLevels();
        }
        break;
    case NONE:
    default:
        break;
    }
}

void MapUI::draw() {
    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);

    // draw the level indicators (bottom to top)
    size_t lvlRecW = offsetX - 8;
    size_t lvlRecH = 18;
    size_t lvlRecX = static_cast<size_t>(x) + 12;
    size_t numLevels = game.currentDungeon->getNumLevels();
    size_t lvlSpacing = 12;
    size_t totalHeight = numLevels * lvlRecH + (numLevels - 1) * lvlSpacing;
    size_t startY = static_cast<size_t>(y) + (static_cast<size_t>(height) - totalHeight) / 2;
    for (size_t lvl = 0; lvl < numLevels; lvl++) {
        size_t lvlRecY = startY + (numLevels - 1 - lvl) * (lvlRecH + lvlSpacing);
        Rectangle r = { (float)lvlRecX, (float)lvlRecY, (float)lvlRecW, (float)lvlRecH };
        DrawRectangleRounded(r, 0.2f, 4, LIGHTBURGUNDY);

        // text on rect
        static char lvlText[32];
        snprintf(lvlText, sizeof(lvlText), "Lvl %d", (int)lvl);

        int fontSize = 10;
        int textWidth = MeasureText(lvlText, fontSize);
        int textX = lvlRecX + (lvlRecW - textWidth) / 2;
        int textY = lvlRecY + (lvlRecH - fontSize) / 2;

        DrawText(lvlText, textX, textY, fontSize, LIGHTGRAY);

        // selection indicator with arrows
        if (lvl == currentLevel) {
            DrawRectangleRoundedLines(r, 0.2f, 4, LIGHTGRAY);

            float centerX = lvlRecX + lvlRecW / 2.0f;
            float arrowSize = 4.0f;

            if (lvl < numLevels - 1) {
                Vector2 v1 = { centerX, (float)lvlRecY - 8 };              // top point
                Vector2 v2 = { centerX - arrowSize, (float)lvlRecY - 3 };  // bottom left
                Vector2 v3 = { centerX + arrowSize, (float)lvlRecY - 3 };  // bottom right
                DrawTriangle(v1, v2, v3, LIGHTGRAY);
            }
            if (lvl > 0) {
                Vector2 v1 = { centerX, (float)lvlRecY + (float)lvlRecH + 8 };          // bottom point
                Vector2 v2 = { centerX - arrowSize, (float)lvlRecY + (float)lvlRecH + 3 }; // top left
                Vector2 v3 = { centerX + arrowSize, (float)lvlRecY + (float)lvlRecH + 3 }; // top right
                DrawTriangle(v1, v3, v2, LIGHTGRAY);
            }
        }
    }

    // draw the room layout
    const auto [cols, rows] = game.currentDungeon->getSize();
    size_t currentRoomIndex = game.currentDungeon->getCurrentRoomIndex();
    const size_t cellWidth = (static_cast<size_t>(width) - 2 * border - (cols - 1) * spacing - offsetX) / cols;
    const size_t cellHeight = (static_cast<size_t>(height) - 2 * border - (rows - 1) * spacing - offsetY) / rows;

    // calculate the minimap offsets for the 4 doors
    // TODO: do I really need to recalculate this every frame?
    offsets[0].x = float(cellWidth);
    offsets[0].y = float(cellHeight / 2 - spacing / 2);
    offsets[1].x = float(cellWidth / 2 - spacing / 2);
    offsets[1].y = -1.0f * float(spacing);
    offsets[2].x = -1.0f * float(spacing);
    offsets[2].y = float(cellHeight / 2 - spacing / 2);
    offsets[3].x = float(cellWidth / 2 - spacing / 2);
    offsets[3].y = float(cellHeight);

    auto& minimaps = game.currentDungeon->minimapTextures;

    for (size_t i = 0; i < cols * rows; ++i) {
        size_t col = i % cols;
        size_t row = i / cols;
        size_t cellX = offsetX + static_cast<size_t>(x) + border + col * (cellWidth + spacing);
        size_t cellY = offsetY + static_cast<size_t>(y) + border + row * (cellHeight + spacing);
        Color color = DARKGRAY;
        DrawRectangle(int(cellX), int(cellY), int(cellWidth), int(cellHeight), color);

        if (i < minimaps[currentLevel].size() && game.currentDungeon->hasVisited(currentLevel, i)) {
            const auto& tex = minimaps[currentLevel][i].texture;
            Rectangle src = { 0, 0, (float)tex.width, (float)tex.height };
            Rectangle dst = { (float)cellX, (float)cellY, (float)cellWidth, (float)cellHeight };
            DrawTexturePro(tex, src, dst, { 0, 0 }, 0.0f, WHITE);

            // indicate the connections between rooms
            uint8_t doors = game.currentDungeon->getRoomDoors(currentLevel, i);

            for (int j = 3; j >= 0; j--) {
                bool isDoor = (doors >> j) & 1;
                if (isDoor) {
                    Rectangle r = {
                        dst.x + offsets[3 - j].x,
                        dst.y + offsets[3 - j].y,
                        float(spacing),
                        float(spacing)
                    };
                    DrawRectangleRec(r, color);
                }
            }

            if (i == currentRoomIndex && game.currentDungeon->getCurrentLevel() == currentLevel && cursorOn && state == OPENED) {
                // draw a player as a blinking circle
                const Vector2& pos = game.getPlayer()->position;
                auto [roomW, roomH] = game.currentDungeon->getRoomSize(currentLevel, currentRoomIndex);
                float u = pos.x / (float)roomW;
                float v = pos.y / (float)roomH;
                float px = cellX + u * cellWidth;
                float py = cellY + v * cellHeight;
                const auto& tex = game.loader.getTextures("knight_map_mini")[0];
                DrawTexture(tex, (int)px, (int)py, WHITE);
            }
        }
    }
    // help texts
    if (state == OPENED) {
        int fontSize = 6;

        static char helpText[32];
        snprintf(helpText, sizeof(helpText), "Level %d", (int)currentLevel);

        const char* textLeft = nullptr;
        //const char* textRight = nullptr; // TODO: draw dynamically if another screen exists
        if (WasGamepadUsedLast()) {
            textLeft = "<< LB";
            //textRight = "RB >>";
        }
        else {
            textLeft = "<< N";
            //textRight = "M >>";
        }
        uint32_t helpTextY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 8;
        //uint32_t txtR = int(x) + int(game.gameScreenWidth) - MeasureText(textRight, fontSize) - 4;
        uint32_t txtL = int(x) + 4;
        //DrawText(textRight, txtR, helpTextY, fontSize, LIGHTGRAY); // TODO: draw dynamically if another screen exists
        DrawText(textLeft, txtL, helpTextY, fontSize, LIGHTGRAY);
        uint32_t helpTextX = int(x) + (int(game.gameScreenWidth) - MeasureText(helpText, fontSize)) / 2;
        DrawText(helpText, helpTextX, helpTextY, fontSize, LIGHTGRAY);
    }
}

void MapUI::end() {
    game.eventManager.pushEvent(SET_MUSIC_VOLUME, 1.0f);
}