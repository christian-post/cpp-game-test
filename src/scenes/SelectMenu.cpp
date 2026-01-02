#include "SelectMenu.h"
#include <functional>

SelectMenu::SelectMenu(Game& game, const std::string& name) 
    : Scene(game, name), menu(MenuSelect(game)) {}

void SelectMenu::startup()
{
    menu.setItems({
        {
            "Back to Game",
            MenuItemType::Action,
            [&]() {
                game.eventManager.pushEvent(SELECT_MENU_DONE);
                game.stopScene(getName());
            }
        },
        {
            "Save Game",
            MenuItemType::Action,
            [&]() {
                game.stopScene(getName());
                game.setOnSceneComplete("WriteSavegameMenu", [&game = game]() {
                    game.eventManager.pushEvent(SELECT_MENU_DONE);
                });
                game.startScene("WriteSavegameMenu");
            }
        },
        {
            "Restart Game", MenuItemType::Action, [&]() {
                game.restart();
            }
        },
        { "Quit to Desktop", MenuItemType::Action, [&]() { game.end(); }}
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
