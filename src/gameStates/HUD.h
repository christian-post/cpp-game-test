#pragma once

#include "Scene.h"
#include <iostream>
#include "raylib.h"
#include <vector>

class HUD : public Scene {
public:
    HUD(Game& game, const std::string& name) : Scene(game, name), heartImages{} {}
    void startup() override;
    void events(const std::unordered_map<std::string, std::any>& events) override;
    void update(float dt) override;
    void draw() override;
    void end() override;

private:
    std::vector<Texture2D> heartImages;
};