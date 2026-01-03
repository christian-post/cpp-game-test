#pragma once
#include "Scene.h"
#include "raylib.h"
#include <iostream>
#include <vector>
#include <array>


class HUD : public Scene {
public:
    HUD(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;

private:
    std::vector<Texture2D> heartImages;
    bool retracting = false; // is in the process of retracting
    bool visible = true; // fully retracted
    float x = 0.0f;
    float y = 0.0f;
    float height = 0.0f; // get from game settings
    float width = 0.0f;

    std::array<std::string, 2> equippedWeapons;
    // feature that display a collected item briefly
    bool showCollectedItem = false;
    int collectedItemY = 0;
    float collectedItemTimer = 0.0f;
    std::string collectedItem;
    // show the amount of coins for shopping
    bool showCoinAmount = false;
    // show a help text for the controls
    bool showHelpText = false;
    std::string helpText;
    char helpTextKey = '\0';
    int helpTextButtonIndex = 0;
};