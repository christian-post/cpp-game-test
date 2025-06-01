#include "InventoryUI.h"
#include "InventoryManager.h"
#include "Game.h"
#include "Controls.h"
#include "raylib.h"
#include "Utils.h"


InventoryUI::InventoryUI(Game& game, const std::string& name) : Scene(game, name) {}

void InventoryUI::startup() {
    y = float(game.gameScreenHeight); // start the inventory hidden at the bottom
    topY = game.getSetting("HudHeight");
    width = game.gameScreenWidth;
    height = game.gameScreenHeight;
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
        if (y < height) {
            y = std::min(static_cast<float>(height), y + deltaTime * speed);
        }
        else {
            game.eventManager.pushEvent("InventoryDone");
        }
        break;
    case OPENED:
        if (game.buttonsPressed & CONTROL_CONFIRM) {
            state = CLOSING;
            game.playSound("menuClose");
        }
        // cursor movement (only when there are weapons)
        auto& items = game.getItems();
        size_t weaponsSize = items[WEAPON].size();
        size_t consumablesSize = items[CONSUMABLE].size();
        size_t totalItems = weaponsSize + consumablesSize;

        if (totalItems == 0) {
            index = 0; // Reset if no items
            break;
        }

        if (index >= totalItems) index = 0; // Clamp

        if (game.buttonsPressed & CONTROL_RIGHT) {
            index = (index + 1) % totalItems;
            game.playSound("menuCursor");
        }
        if (game.buttonsPressed & CONTROL_LEFT) {
            index = (index + totalItems - 1) % totalItems;
            game.playSound("menuCursor");
        }

        if (game.buttonsPressed & CONTROL_ACTION1) {
            if (index < weaponsSize) {
                game.eventManager.pushEvent("weaponSet", items[WEAPON][index].textureKey);
                game.playSound("menuSelect");
            }
            else {
                size_t consumableIndex = index - weaponsSize;
                game.eventManager.pushEvent("consumeItem", items[CONSUMABLE][consumableIndex].textureKey);
            }
        }
        break;
    }
}


void InventoryUI::draw() {
    auto& items = game.getItems();
    size_t weaponsSize = items[WEAPON].size();
    size_t consumablesSize = items[CONSUMABLE].size();
    size_t totalItems = weaponsSize + consumablesSize;

    // background
    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);

    // selectable weapons
    static const uint32_t spacing = 32;
    static const uint32_t marginLeft = 32;
    static const uint32_t marginTop = 32;
    for (size_t i = 0; i < weaponsSize; ++i) {
        size_t row = i / cols;
        size_t col = i % cols;
        const auto& tex = game.loader.getTextures(items[WEAPON][i].textureKey)[0];
        int drawX = int(x + marginLeft + spacing * col - tex.width / 2);
        int drawY = int(y + marginTop + spacing * row - tex.height / 2);
        DrawTexture(tex, drawX, drawY, WHITE);
    }

    // consumables
    uint32_t itemsStartY = weaponsRows * spacing;
    for (size_t i = 0; i < consumablesSize; ++i) {
        size_t row = i / cols;
        size_t col = i % cols;
        const auto& tex = game.loader.getTextures(items[CONSUMABLE][i].textureKey)[0];
        int centerX = int(x + marginLeft + spacing * col);
        int centerY = int(y + itemsStartY + spacing * row);
        int drawX = centerX - tex.width / 2;
        int drawY = centerY - tex.height / 2;
        DrawTexture(tex, drawX, drawY, WHITE);
        std::string qtyText = "x" + std::to_string(items[CONSUMABLE][i].quantity);
        DrawText(qtyText.c_str(), centerX + 4, centerY + 8, 10, LIGHTGRAY);
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
        int cursorY = cursorBaseY + spacing * cursorRow - cursorTex.height / 2;
        DrawTexture(cursorTex, cursorX, cursorY, WHITE);
    }

    // display selected item name
    int fontSize = 8;
    uint32_t textY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 24;
    //uint32_t textY = int(y) + int(game.gameScreenHeight / 2 - topY) - fontSize + 8;
    if (index < weaponsSize) {
        const char* weaponText = items[WEAPON][index].name.c_str();
        uint32_t textX = (int(game.gameScreenWidth) - MeasureText(weaponText, fontSize)) / 2;
        
        DrawText(weaponText, textX, textY, fontSize, LIGHTGRAY);
    }
    else if (index < totalItems) {
        size_t consumableIndex = index - weaponsSize;
        const char* consumableText = items[CONSUMABLE][consumableIndex].name.c_str();
        uint32_t textX = (int(game.gameScreenWidth) - MeasureText(consumableText, fontSize)) / 2;
        DrawText(consumableText, textX, textY, fontSize, LIGHTGRAY);
    }

    // help text
    fontSize = 6;
    const char* helpText = "Equip/use with O";
    uint32_t helpTextX = (int(game.gameScreenWidth) - MeasureText(helpText, fontSize)) / 2;
    uint32_t helpTextY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 8;
    DrawText(helpText, helpTextX, helpTextY, fontSize, LIGHTGRAY);
}

void InventoryUI::end() {
    game.eventManager.pushEvent("setMusicVolume", 1.0f);
}