#include "SelectMenu.h"
#include <functional>

SelectMenu::SelectMenu(Game& game, const std::string& name) 
    : Scene(game, name), menu(MenuSelect(game)) {}

void SelectMenu::startup()
{
    menu.setItems({
        {
            "Back to Game",
            [&]() {
                game.eventManager.pushEvent(SELECT_MENU_DONE);
                game.stopScene(getName());
            }
        },
        {
            "Save Game",
            [&]() {
                game.stopScene(getName());
                game.setOnSceneComplete("WriteSavegameMenu", [&game = game]() {
                    game.eventManager.pushEvent(SELECT_MENU_DONE);
                });
                game.startScene("WriteSavegameMenu");
            }
        },
        {
            "Restart Game", [&]() {
                game.restart();
            }
        },
        { "Quit to Desktop", [&]() { game.end(); }}
    });
}

void SelectMenu::update(float deltaTime)
{
    menu.update();
}

void SelectMenu::draw()
{
    menu.draw();
}
