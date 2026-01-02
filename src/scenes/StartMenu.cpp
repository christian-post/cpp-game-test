#include "StartMenu.h"
#include "Game.h"
#include "Controls.h"
#include "Utils.h"

StartMenu::StartMenu(Game& game, const std::string& name) 
    : Scene(game, name), menu(MenuSelect(game)) {
}

void StartMenu::startup() {
    menu.addItem({
            "New Game", 
            MenuItemType::Action,
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
                MenuItemType::Action,
                [&]() {
                // TODO: Transition to another menu that lets you select a file
                game.startScene("LoadSavegameMenu");
                game.stopScene(getName());
            }
            });
    }

    menu.addItem({
            "Settings",
            MenuItemType::Action,
            [&]() {
                game.setOnSceneComplete("SettingsMenu", [&game = game]() {
                    game.startScene("StartMenu");
                });
                game.startScene("SettingsMenu");
                game.stopScene(getName());
            }
        });

    menu.addItem({
            "Sound Test",
            MenuItemType::Action,
            [&]() {
                game.startScene("SoundTest");
                game.stopScene(getName());
            }
        });

    menu.addItem({
            "Quit", MenuItemType::Action, [&]() { game.end(); }
        });
}

void StartMenu::update(float deltaTime) {
    menu.update();
}

void StartMenu::draw() {
    menu.draw();
}
