#include "raylib.h"
#include "WriteSavegameMenu.h"
#include "Game.h"
#include "Utils.h"
#include <fstream>

WriteSavegameMenu::WriteSavegameMenu(Game& game, const std::string& name)
    : Scene(game, name), menu(MenuSelect(game)) {

    menu.setFontSize(8);
    menu.setMargin(6);
}

void WriteSavegameMenu::startup()
{
    size_t fileIndex = 0;
    std::vector<std::string> files = listFiles("./savegames");
    for (const auto& file : files) {
        // extract the basename to display as the menu item
        // TODO: strip the ".json" also
        size_t pos = file.find("savegames") + 10;
        std::string substr = file.substr(pos);
        size_t idxPos = substr.find("_") + 1;
        if (idxPos) {
            size_t idx = std::stoi(substr.substr(idxPos));
            if (idx > fileIndex)
                fileIndex = idx;
        }

        menu.addItem({ substr, [this, substr]() {
            // save the game under this filename
            game.eventManager.pushEvent(SAVE_GAME, std::any(substr));
            game.eventManager.pushEvent(SELECT_MENU_DONE);
            game.stopScene(getName());
            }
            });
    }
    menu.addItem({ "New Save", [this, fileIndex]() {
        // create a new save
        std::string s = "save_" + std::to_string(fileIndex + 1) + ".json";
        game.eventManager.pushEvent(SAVE_GAME, std::any(s));
        game.eventManager.pushEvent(SELECT_MENU_DONE);
        game.stopScene(getName());
        } });
    // go back to the start menu
    menu.addItem({ "Back", [&]() {
        game.startScene("SelectMenu");
        game.stopScene(getName());
        } });
}

void WriteSavegameMenu::update(float deltaTime)
{
    menu.update();
}

void WriteSavegameMenu::draw()
{
    menu.draw();
}

void WriteSavegameMenu::end()
{
}
