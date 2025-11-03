#include "StartMenu.h"
#include "Game.h"
#include "Controls.h"
#include "Utils.h"
#include "raylib.h"

StartMenu::StartMenu(Game& game, const std::string& name) 
    : Scene(game, name), menu(MenuSelect(game)) {
}

void StartMenu::startup() {
    menu.addItem({
            "New Game",
            [&]() {
                // starts a new game
                game.startScene("InGame");
                game.startScene("HUD");
                game.stopScene(getName());
            }
        });

    if (listFiles("./savegames").size()) {
        menu.addItem({
                "Load Game",
                [&]() {
                // TODO: Transition to another menu that lets you select a file
                game.startScene("LoadSavegameMenu");
                game.stopScene(getName());
            }
            });
    }

    menu.addItem({
            "Sound Test",
            [&]() {
                game.startScene("SoundTest");
                game.stopScene(getName());
            }
        });

    menu.addItem({
            "Quit", [&]() { game.end(); }
        });
}

void StartMenu::update(float deltaTime) {
    menu.update();
}

void StartMenu::draw() {
    menu.draw();
}
