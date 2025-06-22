#include "HUD.h"
#include "Game.h"
#include "raylib.h"
#include "InGame.h"
#include "Utils.h"
#include "ItemData.h"


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

    game.eventManager.addListener("itemAdded", [this](std::any data) {
        collectedItem = std::any_cast<std::string>(data);
        showCollectedItem = true;
        collectedItemTimer = 0.0f;
        });

    game.eventManager.addListener("showCoinAmount", [this](std::any data) {
        showCoinAmount = true;
        });

    game.eventManager.addListener("hideCoinAmount", [this](std::any data) {
        showCoinAmount = false;
        });
}

void HUD::startup() {
    width = float(game.gameScreenWidth);
    heartImages = game.loader.getTextures("hearts");
    height = game.getSetting("HudHeight");
}

void HUD::update(float deltaTime) {
    if (retracting && y > -height) {
        y = std::max(-height, y - deltaTime * height);
        if (y == -height) visible = false;
    }
    else if (!retracting && y < 0.0f) {
        y = std::min(0.0f, y + deltaTime * height);
    }
    // oscillate the item display
    collectedItemY = 8 + static_cast<int>(4.0f * std::sin(collectedItemTimer * 8.0f));
    collectedItemTimer += deltaTime;
    if (collectedItemTimer >= 2.0f) {
        showCollectedItem = false;
    }
}

void HUD::draw() {
    if (!visible) return;

    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);

    // draw player health as hearts
    // TODO draw the hearts ON the reactangle as a texture?
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
    if (!textures.empty()) {
        const auto& wpnTex = textures[0];
        DrawTexture(wpnTex, weaponX - wpnTex.width / 2, weaponY - wpnTex.height / 2, WHITE);        
    }

    // draw the mini map
    auto [cols, rows] = game.currentDungeon->getSize();
    size_t currentRoomIndex = game.currentDungeon->getCurrentRoomIndex();
    const int spacing = 1;
    const int cellWidth = 6;
    const int cellHeight = 4;
    const int mapX = static_cast<int>(game.gameScreenWidth) - static_cast<int>(cols) * (cellWidth + spacing) - 6;
    const int mapY = 6;
    for (size_t i = 0; i < cols * rows; ++i) {
        int col = static_cast<int>(i % cols);
        int row = static_cast<int>(i / cols);
        int cellX = mapX + col * (cellWidth + spacing);
        int cellY = mapY + row * (cellHeight + spacing);
        Color color = DARKGRAY;
        if (i == currentRoomIndex) {
            color = WHITE;
        }
        DrawRectangle(cellX, cellY, cellWidth, cellHeight, color);
    }

    // whenever a collectable item is picked up
    // TODO this break when it's a coin
    if (showCollectedItem) {
        auto& itemData = game.inventory.getItemData();
        auto& invItems = game.inventory.getItems();
        const ItemData& data = itemData.at(collectedItem);
        const Texture2D& itemTex = game.loader.getTextures(data.textureKey)[0];
        int itemX = weaponX + 36;
        DrawTexture(itemTex, itemX, collectedItemY, WHITE);
        ItemType type = data.type;
        uint32_t qty = invItems[type].at(collectedItem).second;
        std::string qtyText = "x" + std::to_string(qty);
        DrawText(qtyText.c_str(), itemX + 8, collectedItemY, 10, LIGHTGRAY);
    }
    if (showCoinAmount) {
        // TODO get rid of repeated code
        const auto& coinTex = game.loader.getTextures("itemDropCoin_idle")[0];
        int coinX = weaponX + 36;
        DrawTexture(coinTex, coinX, 8, WHITE);
        uint32_t qty = game.inventory.getItemQuantity("coin");
        std::string qtyText = "x" + std::to_string(qty);
        DrawText(qtyText.c_str(), coinX + 8, 8, 10, LIGHTGRAY);
    }
}

void HUD::end() {
    // TODO
}