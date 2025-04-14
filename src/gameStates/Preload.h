#pragma once

#include "Scene.h"
#include <iostream>

class Preload : public Scene {
public:
    Preload(const std::string& name) : Scene(name) {}
    void startup(Game* game) override;
    void update(Game* game, float dt) override;
    void draw(Game* game) override;
    void end() override;
};