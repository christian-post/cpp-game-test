#include "Scene.h"
#include "MenuSelect.h"
#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <functional>

class StartMenu : public Scene {
public:
    StartMenu(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    MenuSelect menu;
};