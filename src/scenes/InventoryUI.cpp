#include "InventoryUI.h"
//#include "InventoryManager.h"
#include "Game.h"
#include "Controls.h"
#include "raylib.h"
#include "Utils.h"


InventoryUI::InventoryUI(Game& game, const std::string& name) : Scene(game, name) {}

void InventoryUI::startup() {
    y = float(game.gameScreenHeight); // start the inventory hidden at the bottom
    topY = game.getSetting("HudHeight");
    width = game.gameScreenWidth;
    height = game.gameScreenHeight - game.getSetting("HudHeight");
    // set the sliding speed so that it takes "slideDuration" seconds to expand the inventory
    speed = height / slideDuration;
    state = OPENING;
    game.playSound("menuOpen");
}

void InventoryUI::update(float deltaTime) {
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
            game.stopScene("InventoryUI");
        }
        break;
    case SLIDING_LEFT:
        if (x > static_cast<float>(width) * -1.0f) {
            x = std::max(static_cast<float>(width) * -1.0f, x - deltaTime * speed);
        }
        else {
            game.pauseScene("InventoryUI");
            state = SLIDING_RIGHT;
        }
        break;
    case SLIDING_RIGHT:
        if (x < 0.0f) {
            x = std::min(0.0f, x + deltaTime * speed);
        }
        else {
            game.pauseScene("MapUI");
            state = OPENED;
        }
        break;
    case OPENED:
        if (game.buttonsPressed & CONTROL_CONFIRM) {
            state = CLOSING;
            game.playSound("menuClose");
        }
        if (game.buttonsPressed & CONTROL_ACTIONR) {
            state = SLIDING_LEFT;
            game.playSound("menuOpen");
            game.startScene("MapUI");
        }
        // cursor movement (only when there are weapons)
        auto& items = game.inventory.getItems();
        std::vector<const InventoryItem*> flatItems;
        for (const auto& [key, item] : items[WEAPON]) flatItems.push_back(&item);
        for (const auto& [key, item] : items[CONSUMABLE]) flatItems.push_back(&item);

        size_t totalItems = flatItems.size();
        if (totalItems == 0) {
            index = 0;
            break;
        }
        if (index >= totalItems) index = 0;

        if (game.buttonsPressed & CONTROL_RIGHT) {
            index = (index + 1) % totalItems;
            game.playSound("menuCursor");
        }
        if (game.buttonsPressed & CONTROL_LEFT) {
            index = (index + totalItems - 1) % totalItems;
            game.playSound("menuCursor");
        }

        size_t weaponCount = items[WEAPON].size();
        size_t nextIndex = index + cols;
        size_t col = index % cols; // TODO: not correct for consumables
        if (game.buttonsPressed & CONTROL_DOWN) {
            if (nextIndex < totalItems) {
                index = nextIndex;
            }
            else if (index < weaponCount) {
                size_t candidateIndex = weaponCount + col;
                if (candidateIndex < totalItems) index = candidateIndex;
                else index = totalItems - 1;
            }
            game.playSound("menuCursor");
        }

        if (game.buttonsPressed & CONTROL_UP) {
            size_t weaponCount = items[WEAPON].size();

            if (index >= weaponCount) {
                size_t consumableIndex = index - weaponCount;
                size_t col = consumableIndex % cols;
                size_t secondRowLen = weaponCount > cols ? weaponCount - cols : 0;
                if (secondRowLen == 0) {
                    // All weapons in first row
                    index = (col < weaponCount) ? col : weaponCount - 1;
                }
                else {
                    // Weapons span two rows
                    if (col < secondRowLen) {
                        index = cols + col;
                    }
                    else {
                        index = weaponCount - 1;
                    }
                }
            }
            else if (index >= cols) {
                // In second weapon row → move to first
                index -= cols;
            }
            game.playSound("menuCursor");
        }

        if (game.buttonsPressed & CONTROL_ACTION1) {
            const auto* selected = flatItems[index];
            if (selected->first->type == WEAPON) {
                game.eventManager.pushEvent("weaponSet", selected->first->textureKey);
                game.playSound("menuSelect");
            }
            else {
                game.eventManager.pushEvent("consumeItem", selected->first->textureKey);
            }
        }
        break;
    }
}


void InventoryUI::draw() {
    auto& items = game.inventory.getItems();
    size_t weaponsSize = items[WEAPON].size();
    size_t consumablesSize = items[CONSUMABLE].size();
    size_t passiveSize = items[PASSIVE].size();
    size_t keySize = items[KEY].size();
    size_t totalItems = weaponsSize + consumablesSize + passiveSize + keySize;

    // background
    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);
    static const uint32_t spacing = 32;
    static const uint32_t marginLeft = 24;
    static const uint32_t marginTop = 24;
    Rectangle r1 = { x + marginLeft * 0.5f, y + marginTop * 0.5f, float(spacing * cols), float(spacing * weaponsRows) };
    Rectangle r2 = { r1.x, y + marginTop + float(spacing * weaponsRows), float(spacing * cols), float(spacing) };
    Rectangle r3 = { x + float(cols) * float(spacing) + 2.0f * float(marginLeft), r1.y, float(spacing), r1.height + r2.height + marginTop * 0.5f };
    DrawRectangleRounded(r1, 0.2f, 0, LIGHTBURGUNDY);
    DrawRectangleRounded(r2, 0.2f, 0, LIGHTBURGUNDY);
    DrawRectangleRounded(r3, 0.2f, 0, LIGHTBURGUNDY);

    std::vector<const InventoryItem*> flatItems;
    for (const auto& [key, item] : items[WEAPON]) flatItems.push_back(&item);
    for (const auto& [key, item] : items[CONSUMABLE]) flatItems.push_back(&item);
    for (const auto& [key, item] : items[PASSIVE]) flatItems.push_back(&item);
    for (const auto& [key, item] : items[KEY]) flatItems.push_back(&item);

    // selectable weapons
    
    for (size_t i = 0; i < weaponsSize; ++i) {
        size_t row = i / cols;
        size_t col = i % cols;
        const auto& tex = game.loader.getTextures(flatItems[i]->first->textureKey)[0];
        int drawX = int(x + marginLeft + spacing * col - tex.width / 2);
        int drawY = int(y + marginTop + spacing * row - tex.height / 2);
        DrawTexture(tex, drawX, drawY, WHITE);
    }

    // consumables
    uint32_t itemsStartY = (weaponsRows + 1) * spacing;
    for (size_t i = weaponsSize; i < weaponsSize + consumablesSize; ++i) {
        size_t row = (i - weaponsSize) / cols;
        size_t col = (i - weaponsSize) % cols;
        const auto& tex = game.loader.getTextures(flatItems[i]->first->textureKey)[0];
        int centerX = int(x + marginLeft + spacing * col);
        int centerY = int(y + itemsStartY + spacing * row);
        int drawX = centerX - tex.width / 2;
        int drawY = centerY - tex.height / 2;
        DrawTexture(tex, drawX, drawY, WHITE);
        std::string qtyText = "x" + std::to_string(flatItems[i]->second);
        DrawText(qtyText.c_str(), centerX + 4, centerY + 8, 10, LIGHTGRAY);
    }

    // passive items, draw as column
    uint32_t passivesStartX = int(x) + cols * spacing + 2 * marginLeft;
    for (size_t i = weaponsSize + consumablesSize; i < weaponsSize + consumablesSize + passiveSize; ++i) {
        const auto& tex = game.loader.getTextures(flatItems[i]->first->textureKey)[0];
        int centerX = int(passivesStartX);
        int centerY = int(marginTop + y + (i - weaponsSize - consumablesSize) * spacing);
        int drawX = centerX + tex.width / 2;
        int drawY = centerY - tex.height / 2;
        DrawTexture(tex, drawX, drawY, WHITE);
        std::string qtyText = "x" + std::to_string(flatItems[i]->second);
        DrawText(qtyText.c_str(), centerX + 8, centerY + 8, 10, LIGHTGRAY);
    }

    // draw the cursor
    if (index < totalItems) {
        size_t cursorRow, cursorCol;
        int cursorBaseY;
        if (index < weaponsSize) {
            cursorRow = index / cols;
            cursorCol = index % cols;
            cursorBaseY = int(y + marginTop);
        }
        else {
            size_t consumableIndex = index - weaponsSize;
            cursorRow = consumableIndex / cols;
            cursorCol = consumableIndex % cols;
            cursorBaseY = int(y + itemsStartY);
        }
        const auto& cursorTex = game.loader.getTextures("inventory_cursor")[0];
        int cursorX = int(x + marginLeft + spacing * cursorCol - cursorTex.width / 2);
        int cursorY = cursorBaseY + spacing * (int)cursorRow - cursorTex.height / 2;
        DrawTexture(cursorTex, cursorX, cursorY, WHITE);
    }

    // display selected item name
    int fontSize = 8;
    uint32_t textY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 24;
    if (index < weaponsSize) {
        const char* weaponText = flatItems[index]->first->displayName.c_str();
        uint32_t textX = int(x) + (int(game.gameScreenWidth) - MeasureText(weaponText, fontSize)) / 2;
        
        DrawText(weaponText, textX, textY, fontSize, LIGHTGRAY);
    }
    else if (index < totalItems) {
        size_t consumableIndex = index - weaponsSize;
        const char* consumableText = flatItems[index]->first->displayName.c_str();
        uint32_t textX = int(x) + (int(game.gameScreenWidth) - MeasureText(consumableText, fontSize)) / 2;
        DrawText(consumableText, textX, textY, fontSize, LIGHTGRAY);
    }

    // help texts
    if (state == OPENED) {
        fontSize = 6;
        const char* helpText = nullptr;
        const char* textLeft = nullptr;
        const char* textRight = nullptr;
        if (WasGamepadUsedLast()) {
            helpText = "Equip/use with [A]"; // TODO draw a sprite that shows the buttons
            textLeft = "<< LB";
            textRight = "RB >>";
        }
        else {
            helpText = "Equip/use with [O]";
            textLeft = "<< N";
            textRight = "M >>";
        }
        uint32_t helpTextX = int(x) + (int(game.gameScreenWidth) - MeasureText(helpText, fontSize)) / 2;
        uint32_t helpTextY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 8;
        DrawText(helpText, helpTextX, helpTextY, fontSize, LIGHTGRAY);

        uint32_t txtR = int(x) + int(game.gameScreenWidth) - MeasureText(textRight, fontSize) - 4;
        uint32_t txtL = int(x) + 4;
        DrawText(textRight, txtR, helpTextY, fontSize, LIGHTGRAY);
        DrawText(textLeft, txtL, helpTextY, fontSize, LIGHTGRAY);
    }
}

void InventoryUI::end() {
    game.eventManager.pushEvent("setMusicVolume", 1.0f);
}