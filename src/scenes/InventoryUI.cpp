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
}

void InventoryUI::update(float dt) {
    switch (state) {
    case OPENING:
        if (y > topY) {
            y = std::max(topY, y - dt * speed);
        }
        else {
            state = OPENED;
        }
        break;
    case CLOSING:
        if (y < height) {
            y = std::min(static_cast<float>(height), y + dt * speed);
        }
        else {
            game.eventManager.pushEvent("InventoryDone");
        }
        break;
    case OPENED:
        if (game.buttonsPressed & CONTROL_CONFIRM) {
            state = CLOSING;
        }
        // cursor movement (only when there are weapons)
        auto& items = game.getItems();
        if (!items[WEAPON].size()) break;
        if (game.buttonsPressed & CONTROL_RIGHT) {
            index = (index + 1) % items[WEAPON].size();
        }
        if (game.buttonsPressed & CONTROL_LEFT) {
            index = (index + items[WEAPON].size() - 1) % items[WEAPON].size();
        }
        if (game.buttonsPressed & CONTROL_ACTION1) {
            game.eventManager.pushEvent("weaponSet", items[WEAPON][index].textureKey);
        }
        break;
    }
}


void InventoryUI::draw() {
    // background
    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);
    // selectable weapons
    uint32_t spacing = 32;
    uint32_t marginLeft = 48;
    uint32_t marginTop = 32;
    auto& items = game.getItems();
    for (size_t i = 0; i < items[WEAPON].size(); ++i) {
        size_t row = i / cols;
        size_t col = i % cols;
        const auto& tex = game.loader.getTextures(items[WEAPON][i].textureKey)[0];
        int drawX = int(x + marginLeft + spacing * col - tex.width / 2);
        int drawY = int(y + marginTop + spacing * row - tex.height / 2);
        DrawTexture(tex, drawX, drawY, WHITE);
    }
    // all the consumable items
    uint32_t itemsStartY = weaponsRows * spacing;
    for (size_t i = 0; i < items[CONSUMABLE].size(); ++i) {
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
    // draw a cursor to select a weapon
    size_t cursorRow = index / cols;
    size_t cursorCol = index % cols;
    const auto& cursorTex = game.loader.getTextures("inventory_cursor")[0];
    int cursorX = int(x + marginLeft + spacing * cursorCol - cursorTex.width / 2);
    int cursorY = int(y + marginTop + spacing * cursorRow - cursorTex.height / 2);
    DrawTexture(cursorTex, cursorX, cursorY, WHITE);

    if (items[WEAPON].size() > 0) {
        const char* weaponText = items[WEAPON][index].name.c_str();
        int fontSize = 8;
        uint32_t textX = (int(game.gameScreenWidth) - MeasureText(weaponText, fontSize)) / 2; // center text
        uint32_t textY = int(y) + int(game.gameScreenHeight / 2 - topY) - fontSize + 8;
        DrawText(weaponText, textX, textY, fontSize, LIGHTGRAY);
    }

    int fontSize = 6;
    const char* helpText = "Equip with O";
    uint32_t helpTextX = (int(game.gameScreenWidth) - MeasureText(helpText, fontSize)) / 2; // center text
    uint32_t helpTextY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 8;
    DrawText(helpText, helpTextX, helpTextY, fontSize, LIGHTGRAY);

}

void InventoryUI::end() {
    game.eventManager.pushEvent("setMusicVolume", 1.0f);
}