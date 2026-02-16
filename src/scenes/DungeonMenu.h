#pragma once
#include "Scene.h"
#include "MenuSelect.h"

enum class MenuState
{
    Idle,
    Generating,
    Failed
};

std::string seedToHexString(uint32_t seed); // converts an integer seed into an 8 character hex value

class DungeonMenu : public Scene 
{
public:
    DungeonMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;

private:
    MenuSelect menu;
    std::string displayMessage = "";
    std::string lastSeedMessage = "";
    MenuState currentState = MenuState::Idle;
};