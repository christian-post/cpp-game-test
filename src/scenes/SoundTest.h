#pragma once
#include "Scene.h"
#include "MenuSelect.h"
#include <functional>

class SoundTest : public Scene {
public:
    SoundTest(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    MenuSelect menu;
    Sound currentSound;
};