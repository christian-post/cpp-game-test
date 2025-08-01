#include "StartMenu.h"
#include "Game.h"
#include "Controls.h"
#include "raylib.h"

StartMenu::StartMenu(Game& game, const std::string& name) 
    : Scene(game, name), menu(MenuSelect(game)) {

    game.eventManager.addListener("loadingSavegameSuccess", [&](const std::any& data) {
            game.startScene("InGame");
            game.startScene("HUD");
            game.stopScene(getName());
        });
}

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
        { 
            "Load Game", 
            [&]() {
                // TODO go to another menu that lets you select a file
                game.eventManager.pushEvent("loadGame");
            }
        },
        { "Quit", [&]() { game.end(); }}
        });
}

void StartMenu::update(float deltaTime) {
    menu.update();
}

void StartMenu::draw() {
    menu.draw();
}
