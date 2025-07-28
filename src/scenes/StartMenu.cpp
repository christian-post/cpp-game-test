#include "StartMenu.h"
#include "Game.h"
#include "Controls.h"
#include "raylib.h"

StartMenu::StartMenu(Game& game, const std::string& name) 
    : Scene(game, name), menu(MenuSelect(game)) {}

void StartMenu::startup() {
    menu.setItems({
        { 
            "New Game", 
            [&]() {
                // starts a new game
                game.startScene("InGame");
                game.startScene("HUD");
                game.stopScene(getName());
            }
        },
        { "Load Game", nullptr }, // TODO
        { "Quit", [&]() { game.end(); }}
        });
}

void StartMenu::update(float deltaTime) {
    menu.update();
}

void StartMenu::draw() {
    menu.draw();
}
