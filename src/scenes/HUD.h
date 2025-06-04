#pragma once

#include "Scene.h"
#include <iostream>
#include "raylib.h"
#include <vector>

class HUD : public Scene {
public:
    HUD(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    std::vector<Texture2D> heartImages;
    bool retracting = false; // is in the process of retracting
    bool visible = true; // fully retracted
    float x = 0.0f;
    float y = 0.0f;
    float height = 0.0f; // get from game settings
    float width = 0.0f;

    std::string equippedWeapon;
    // feature that display a collected item briefly
    bool showCollectedItem = false;
    int collectedItemY = 0;
    float collectedItemTimer = 0.0f;
    std::string collectedItem = "";
    // show the amount of coins for shopping
    bool showCoinAmount = false;
};