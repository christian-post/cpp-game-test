#pragma once

#include "Scene.h"
#include <iostream>

class Preload : public Scene {
public:
    Preload(Game& game, const std::string& name) : Scene(game, name) {}
    void startup() override;
    void update(float dt) override;
    void draw() override;
    void end() override;
};