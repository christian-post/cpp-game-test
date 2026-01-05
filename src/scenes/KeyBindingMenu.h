#pragma once
#include "Scene.h"
#include "MenuSelect.h"

class KeyBindingMenu : public Scene
{
public:
    KeyBindingMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;

private:
    MenuSelect menu;

    struct ControlBinding
    {
        std::string name;
        size_t arrayIndex;
        size_t keyIndex; // 0 or 1 for primary/secondary
        bool isGamepad;  // true = save to gamepadBindings, false = keyBindings
    };

    void saveBinding(size_t arrayIndex, size_t keyIndex, int keycode, bool isGamepad);
};