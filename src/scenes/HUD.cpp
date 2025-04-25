#include "HUD.h"
#include "Game.h"
#include "raylib.h"
#include "InGame.h"
#include "Utils.h"

void HUD::startup() {
    width = float(game.gameScreenWidth);
    heartImages = game.loader.getTextures("hearts");
    height = game.getSetting("HudHeight");
}

void HUD::events(const std::unordered_map<std::string, std::any>& events) {
    for (const auto& [name, data] : events) {
        if (name == "hideHUD") {
            // start hiding the HUD
            if (visible && !retracting) retracting = true;
        }
        else if (name == "showHUD") {
            // sliding in the HUD
            if (!visible && retracting) {
                retracting = false;
                visible = true;
            }
        }
        else if (name == "weaponSet") {
            equippedWeapon = std::any_cast<std::string>(data);
            Log("player got the " + equippedWeapon);
        }
    }
}

void HUD::update(float dt) {
    if (retracting && y > -height) {
        y = std::max(-height, y - dt * height);
        if (y == -height) visible = false;
    }
    else if (!retracting && y < 0.0f) {
        y = std::min(0.0f, y + dt * height);
    }
}

void HUD::draw() {
    if (!visible) return;

    DrawRectangle(int(x), int(y), int(width), int(height), BLACK);

    // draw player health as hearts
    // TODO draw the hearts ON the reactangle as a texture
    Sprite* player = game.getPlayer();
    if (player) {
        int spacing = heartImages[0].width + 2;
        int totalHearts = player->maxHealth / 2;
        int hp = player->health;
        for (int i = 0; i < totalHearts; i++) {
            int imgIndex = (hp >= 2) ? 2 : (hp == 1 ? 1 : 0);
            DrawTexture(heartImages[imgIndex], 8 + spacing * i, int(y) + 8, WHITE);
            hp -= 2;
        }
    }

    // draw the currently equipped weapon
    // TODO: correctly calculate how the item texture gets centered on the rectangle
    int weaponX = int(x) + int(game.gameScreenWidth * 2 / 3);
    DrawRectangleLines(weaponX - 6, int(y) + 4, 24, 24, LIGHTGRAY); // background
    DrawTexture(game.loader.getTextures(equippedWeapon)[0], weaponX, int(y) + 4, WHITE);
}

void HUD::end() {

}