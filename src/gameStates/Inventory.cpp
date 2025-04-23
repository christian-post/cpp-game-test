#include "Inventory.h"
#include "Game.h"
#include "Controls.h"
#include "raylib.h"
#include "Utils.h"



void Inventory::startup() {
    y = float(game.gameScreenHeight); // start the inventory hidden at the bottom
    topY = game.getSetting("HudHeight");
    width = game.gameScreenWidth;
    height = game.gameScreenHeight;
    // set the sliding speed so that it takes "slideDuration" seconds to expand the inventory
    speed = height / slideDuration;
    state = OPENING;

    // TODO some items for testing
    // TODO split items in equippables and "other" (like arrows)
    weapons.emplace_back("Sword", 1, "weapon_sword");
    weapons.emplace_back("Bow", 1, "weapon_bow");
    weapons.emplace_back("Hammer", 1, "weapon_hammer");
    weapons.emplace_back("Double axe", 1, "weapon_double_axe");
    weapons.emplace_back("Mace", 1, "weapon_mace");
    weapons.emplace_back("Spear", 1, "weapon_spear");

    items.emplace_back("Arrows", 69, "weapon_arrow");
    items.emplace_back("Red Potion", 2, "flask_big_red");
}

void Inventory::events(const std::unordered_map<std::string, std::any>& events) {
    
}

void Inventory::update(float dt) {
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
        if (game.buttonsPressed & CONTROL_RIGHT) {
            index = (index + 1) % weapons.size();
        }
        if (game.buttonsPressed & CONTROL_LEFT) {
            index = (index + weapons.size() - 1) % weapons.size();
        }
        if (game.buttonsPressed & CONTROL_ACTION1) {
            game.eventManager.pushEvent("weaponSet", weapons[index].textureKey);
        }
        break;
    }
}


void Inventory::draw() {
    // black background
    DrawRectangle(int(x), int(y), int(width), int(height), BLACK);
    // selectable weapons
    uint32_t spacing = 32;
    uint32_t marginLeft = 32;
    uint32_t marginTop = 16;
    for (size_t i = 0; i < weapons.size(); ++i) {
        size_t row = i / cols;
        size_t col = i % cols;
        int drawX = int(x + marginLeft + spacing * col);
        int drawY = int(y + marginTop + spacing * row);
        DrawTexture(game.loader.getTextures(weapons[i].textureKey)[0], drawX, drawY, WHITE);
    }
    // all the consumable items
    uint32_t itemsStartY = weaponsRows * spacing;
    for (size_t i = 0; i < items.size(); ++i) {
        size_t row = i / cols;
        size_t col = i % cols;
        int drawX = int(x + marginLeft + spacing * col);
        int drawY = int(y + itemsStartY + spacing * row);
        DrawTexture(game.loader.getTextures(items[i].textureKey)[0], drawX, drawY, WHITE);
        std::string qtyText = "x" + std::to_string(items[i].quantity);
        DrawText(qtyText.c_str(), drawX + 8, drawY + 16, 10, LIGHTGRAY);
    }
    // draw a cursor to select a weapon
    size_t cursorRow = index / cols;
    size_t cursorCol = index % cols;
    DrawRectangleLines(
        int(x + marginLeft + spacing * cursorCol),
        int(y + marginTop + spacing * cursorRow),
        28, 28, LIGHTGRAY
    );

    const char* weaponText = weapons[index].name.c_str();
    int fontSize = 16;
    int titleWidth = MeasureText(weaponText, fontSize);
    uint32_t titleX = (int(game.gameScreenWidth) - titleWidth) / 2;
    uint32_t titleY = int(y) + int(game.gameScreenHeight - topY) - fontSize - 8;
    DrawText(weaponText, titleX, titleY, fontSize, LIGHTGRAY);
}

void Inventory::end() {

}