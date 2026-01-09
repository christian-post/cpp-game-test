#include "HUD.h"
#include "Game.h"
#include "InGame.h"
#include "Utils.h"
#include "ItemData.h"
#include "Controls.h"
#include <cstdint>

HUD::HUD(Game& game, const std::string& name) : Scene(game, name), heartImages{}
{
    // event listeners
    game.eventManager.addListener(HIDE_HUD, [this](std::any) {
        // start hiding the HUD
        if (visible && !retracting) retracting = true;
        });

    game.eventManager.addListener(SHOW_HUD, [this](std::any) {
        // sliding in the HUD
        if (!visible && retracting) {
            retracting = false;
            visible = true;
        }
        });

    game.eventManager.addListener(WEAPON_SET, [this](std::any data) {
        auto [weapon, idx] = std::any_cast<std::pair<std::string, size_t>>(data);
        equippedWeapons[idx] = weapon;
        // unequip if the same weapon happens to be in the other slot
        if (equippedWeapons[(idx + 1) % 2] == weapon) {
            equippedWeapons[(idx + 1) % 2] = "";
        }
        TraceLog(LOG_INFO, "player equipped the %s in slot %d", weapon.c_str(), idx);
        });

    game.eventManager.addListener(ITEM_ADDED, [this](std::any data) {
        collectedItem = std::any_cast<std::string>(data);
        showCollectedItem = true;
        collectedItemTimer = 0.0f;
        });

    game.eventManager.addListener(SHOW_COIN_AMOUNT, [this](std::any data) {
        showCoinAmount = true;
        });

    game.eventManager.addListener(HIDE_COIN_AMOUNT, [this](std::any data) {
        showCoinAmount = false;
        });

    game.eventManager.addListener(SHOW_HELP_TEXT, [this](std::any data) {
        if (showHelpText) 
            return; // already shows the text; do nothing
        auto [label, key, index] = std::any_cast<std::tuple<std::string, char, int>>(data);
        helpText = label;
        helpTextKey = key;
        helpTextButtonIndex = index;
        showHelpText = true;
        });

    game.eventManager.addListener(HIDE_HELP_TEXT, [this](std::any data) {
        showHelpText = false;
        });
}

void HUD::startup()
{
    width = float(game.gameScreenWidth);
    heartImages = game.loader.getTextures("hearts");
    height = game.getSetting<float>("HudHeight");
}

void HUD::update(float deltaTime)
{
    if (retracting && y > -height)
    {
        y = std::max(-height, y - deltaTime * height);
        if (y == -height) 
            visible = false;
    }
    else if (!retracting && y < 0.0f)
    {
        y = std::min(0.0f, y + deltaTime * height);
    }
    // oscillate the item display
    collectedItemY = 8 + static_cast<int>(4.0f * std::sin(collectedItemTimer * 8.0f));
    collectedItemTimer += deltaTime;
    if (collectedItemTimer >= 2.0f)
        showCollectedItem = false;
}

void HUD::draw()
{
    if (!visible) 
        return;

    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY);

    // draw player health as hearts
    // TODO draw the hearts ON the reactangle as a texture?
    Sprite* player = game.getPlayer();
    if (player)
    {
        int spacing = heartImages[0].width + 2;
        int totalHearts = player->maxHealth / 2;
        int hp = player->health;
        for (int i = 0; i < totalHearts; i++)
        {
            int imgIndex = (hp >= 2) ? 2 : (hp == 1 ? 1 : 0);
            DrawTexture(heartImages[imgIndex], 8 + spacing * i, int(y) + 8, WHITE);
            hp -= 2;
        }
    }
    // draw the currently equipped weapons on a background texture frame
    const auto& frameTex = game.loader.getTextures("inventory_item_frame")[0];
    const size_t weaponSlotMargin = static_cast<size_t>(frameTex.width) + 8;
    int weaponX = int(x) + int(static_cast<float>(game.gameScreenWidth) * 0.6); // TODO calculate this once instead of every frame
    const int weaponY = int(y) + 16;
    for (size_t slot = 0; slot < 2; slot++)
    {
        // draw background
        weaponX += int(slot * weaponSlotMargin);
        DrawTexture(frameTex, weaponX - frameTex.width / 2, weaponY - frameTex.height / 2, WHITE);
        // draw weapon
        auto& textures = game.loader.getTextures(equippedWeapons[slot]);
        if (!textures.empty())
        {
            const auto& wpnTex = textures[0];
            DrawTexture(wpnTex, weaponX - wpnTex.width / 2, weaponY - wpnTex.height / 2, WHITE);
        }
        // show the corresponding button or key
        if (WasGamepadUsedLast())
        {
            int button = game.getGamepadButtonForControl((slot == 0) ? CONTROL_ACTION2 : CONTROL_ACTION3);
            const auto& buttonTex = game.loader.getTextures("xbox_buttons_sorted")[button];
            DrawTexture(buttonTex, weaponX + frameTex.width / 2 - 12, weaponY, WHITE);
        }
        else
        {
            std::string btnText = slot == 0 ? "P" : "L";
            DrawText(btnText.c_str(), weaponX + frameTex.width / 2 - 6, weaponY + 8, 10, LIGHTGRAY);
        }
    }

    // draw the mini map
    game.currentWorld->renderMinimap(y, game.gameScreenWidth);

    // whenever a collectable item is picked up
    if (showCollectedItem)
    {
        auto& itemData = game.inventory.getItemData();
        auto& invItems = game.inventory.getItems();
        const ItemData& data = itemData.at(collectedItem);
        ItemType type = data.type;
        if (data.type != IMMEDIATE)
        {
            const Texture2D& itemTex = game.loader.getTextures(data.textureKey)[0];
            int itemX = 112;
            DrawTexture(itemTex, itemX, collectedItemY, WHITE);
            uint32_t qty = invItems[type].at(collectedItem).second;
            std::string qtyText = "x" + std::to_string(qty);
            DrawText(qtyText.c_str(), itemX + 8, collectedItemY, 10, LIGHTGRAY);
        }
    }

    if (showCoinAmount)
    {
        // TODO get rid of repeated code
        const auto& coinTex = game.loader.getTextures("itemDropCoin")[0];
        int coinX = weaponX + 36;
        DrawTexture(coinTex, coinX, 8, WHITE);
        uint32_t qty = game.inventory.getItemQuantity("coin");
        std::string qtyText = "x" + std::to_string(qty);
        DrawText(qtyText.c_str(), coinX + 8, 8, 10, LIGHTGRAY);
    }

    if (showHelpText)
    {
        // context sensitive help text at the bottom of the screen
        const char* ht = helpText.c_str();
        int fontSize = 10;
        int margin = 2;
        int txtPosX = 12;
        int txtPosY = static_cast<int>(game.gameScreenHeight) - 2 * margin - fontSize;
        int txtH = fontSize + 2 * margin;
        if (WasGamepadUsedLast())
        {
            // show the respective button texture
            const auto& buttonTex = game.loader.getTextures("xbox_buttons_sorted")[helpTextButtonIndex];
            int txtW = MeasureText(ht, fontSize) + 2 * margin + buttonTex.width;
            DrawRectangle(txtPosX, txtPosY, txtW, txtH, BLACK);
            DrawTexture(buttonTex, txtPosX, txtPosY - 2, WHITE);
            txtPosX += buttonTex.width;
            DrawText(ht, txtPosX + margin, txtPosY + margin, fontSize, LIGHTGRAY);
        }
        else
        {
            // show a text with the respective key
            std::string displayText = helpText;
            displayText = "[" + std::string(1, helpTextKey) + "]: " + helpText;
            int txtW = MeasureText(displayText.c_str(), fontSize) + 2 * margin;
            DrawRectangle(txtPosX, txtPosY, txtW, txtH, BLACK);
            DrawText(displayText.c_str(), txtPosX + margin, txtPosY + margin, fontSize, LIGHTGRAY);
        }
    }
}