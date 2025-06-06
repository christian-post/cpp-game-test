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
        // TODO: is this really the way to do it?
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

    std::vector<const InventoryItem*> flatItems;
    for (const auto& [key, item] : items[WEAPON]) flatItems.push_back(&item);
    for (const auto& [key, item] : items[CONSUMABLE]) flatItems.push_back(&item);
    for (const auto& [key, item] : items[PASSIVE]) flatItems.push_back(&item);
    for (const auto& [key, item] : items[KEY]) flatItems.push_back(&item);

    // selectable weapons
    static const uint32_t spacing = 32;
    static const uint32_t marginLeft = 24;
    static const uint32_t marginTop = 24;
    for (size_t i = 0; i < weaponsSize; ++i) {
        size_t row = i / cols;
        size_t col = i % cols;
        const auto& tex = game.loader.getTextures(flatItems[i]->first->textureKey)[0];
        int drawX = int(x + marginLeft + spacing * col - tex.width / 2);
        int drawY = int(y + marginTop + spacing * row - tex.height / 2);
        DrawTexture(tex, drawX, drawY, WHITE);
    }

    // consumables
    uint32_t itemsStartY = weaponsRows * spacing;
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
    uint32_t passivesStartX = cols * spacing + 2 * marginLeft; 
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
        int cursorY = cursorBaseY + spacing * cursorRow - cursorTex.height / 2;
        DrawTexture(cursorTex, cursorX, cursorY, WHITE);
    }

    // display selected item name
    int fontSize = 8;
    uint32_t textY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 24;
    if (index < weaponsSize) {
        const char* weaponText = flatItems[index]->first->displayName.c_str();
        uint32_t textX = (int(game.gameScreenWidth) - MeasureText(weaponText, fontSize)) / 2;
        
        DrawText(weaponText, textX, textY, fontSize, LIGHTGRAY);
    }
    else if (index < totalItems) {
        size_t consumableIndex = index - weaponsSize;
        const char* consumableText = flatItems[index]->first->displayName.c_str();
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