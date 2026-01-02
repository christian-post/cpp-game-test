#pragma once
#include "Scene.h"
#include "MenuSelect.h"

class SettingsMenu : public Scene
{
public:
    SettingsMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;

private:
    MenuSelect menu;
};