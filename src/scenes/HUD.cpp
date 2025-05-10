#include "HUD.h"
#include "Game.h"
#include "raylib.h"
#include "InGame.h"
#include "Utils.h"


HUD::HUD(Game& game, const std::string& name) : Scene(game, name), heartImages{} {
    // event listeners
    game.eventManager.addListener("hideHUD", [this](std::any) {
        // start hiding the HUD
        if (visible && !retracting) retracting = true;
        });

    game.eventManager.addListener("showHUD", [this](std::any) {
        // sliding in the HUD
        if (!visible && retracting) {
            retracting = false;
            visible = true;
        }
        });

    game.eventManager.addListener("weaponSet", [this](std::any data) {
        std::string weapon = std::any_cast<std::string>(data);
        equippedWeapon = std::any_cast<std::string>(data);
        TraceLog(LOG_INFO, "player equipped the %s", equippedWeapon.c_str());
        });
}

void HUD::startup() {
    width = float(game.gameScreenWidth);
    heartImages = game.loader.getTextures("hearts");
    height = game.getSetting("HudHeight");
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

    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);

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
    // draw the currently equipped weapon on a background texture frame
    int weaponX = int(x) + int(game.gameScreenWidth * 2 / 3);
    int weaponY = int(y) + 16;
    const auto& frameTex = game.loader.getTextures("inventory_item_frame")[0];
    DrawTexture(frameTex, weaponX - frameTex.width / 2, weaponY - frameTex.height / 2, WHITE);
    auto& textures = game.loader.getTextures(equippedWeapon);
    if (textures.empty()) {
        return;
    }
    const auto& wpnTex = textures[0];
    DrawTexture(wpnTex, weaponX - wpnTex.width / 2, weaponY - wpnTex.height / 2, WHITE);
}

void HUD::end() {

}