#pragma once

#include "Scene.h"
#include <iostream>
#include "raylib.h"
#include <vector>

class HUD : public Scene {
public:
    HUD(const std::string& name) : Scene(name), heartImages{} {}
    void startup(Game* game) override;
    void events(Game* game) override;
    void update(Game* game, float dt) override;
    void draw(Game* game) override;
    void end() override;

private:
    std::vector<Texture2D> heartImages;
};