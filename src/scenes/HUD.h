#pragma once

#include "Scene.h"
#include <iostream>
#include "raylib.h"
#include <vector>

class HUD : public Scene {
public:
    HUD(Game& game, const std::string& name);
    void startup() override;
    //void events(const std::unordered_map<std::string, std::any>& events) override;
    void update(float dt) override;
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
};