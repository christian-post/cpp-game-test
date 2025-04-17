#include "HUD.h"
#include "Game.h"
#include "raylib.h"
#include "InGame.h"

void HUD::startup() {
    heartImages = game.loader.getTextures("hearts");
}

void HUD::events(const std::unordered_map<std::string, std::any>& events) {

}

void HUD::update(float dt) {

}

void HUD::draw() {
    float x = 0.0f;
    float y = 0.0f;
    float height = 32.0f;
    float width = float(game.gameScreenWidth);
    DrawRectangle(int(x), int(y), int(width), int(height), BLACK);

    // draw player health as hearts
    Sprite* player = game.getPlayer();
    if (player) {
        int spacing = heartImages[0].width + 2;
        int totalHearts = player->maxHealth / 2;
        int hp = player->health;
        for (int i = 0; i < totalHearts; i++) {
            int imgIndex = (hp >= 2) ? 2 : (hp == 1 ? 1 : 0);
            DrawTexture(heartImages[imgIndex], 8 + spacing * i, 8, WHITE);
            hp -= 2;
        }
    }
}

void HUD::end() {

}