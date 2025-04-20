#include "HUD.h"
#include "Game.h"
#include "raylib.h"
#include "InGame.h"
#include "Utils.h"

void HUD::startup() {
    width = float(game.gameScreenWidth);
    heartImages = game.loader.getTextures("hearts");
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
    }
}

void HUD::update(float dt) {
    if (retracting && y > -32.0f) {
        y = std::max(-32.0f, y - dt * 32.0f);
        if (y == -32.0f) visible = false;
    }
    else if (!retracting && y < 0.0f) {
        y = std::min(0.0f, y + dt * 32.0f);
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
}

void HUD::end() {

}