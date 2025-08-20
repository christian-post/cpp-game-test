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
                game.eventManager.pushEvent(SAVE_GAME);
                game.eventManager.pushEvent(SELECT_MENU_DONE);
                game.stopScene(getName());
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
