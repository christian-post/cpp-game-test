#include "WriteSavegameMenu.h"
#include "Game.h"
#include "Utils.h"
#include <fstream>

WriteSavegameMenu::WriteSavegameMenu(Game& game, const std::string& name)
    : Scene(game, name), menu(MenuSelect(game)) {

    menu.setFontSize(8);
    menu.setYMargin(6);
}

void WriteSavegameMenu::startup()
{
    size_t fileIndex = 0;

    for (auto& entry : fs::directory_iterator("./savegames")) {
        if (entry.path().extension() == ".json") {
            // extract the basename
            std::string substr = entry.path().stem().string();
            size_t idxPos = substr.find("_") + 1;
            if (idxPos) {
                size_t idx = std::stoi(substr.substr(idxPos));
                if (idx > fileIndex)
                    fileIndex = idx;
            }

            menu.addItem({ substr, MenuItemType::Action, [this, substr]() {
                // save the game under this filename
                game.eventManager.pushEvent(SAVE_GAME, std::any(substr));
                game.eventManager.pushEvent(SELECT_MENU_DONE);
                game.stopScene(getName());
                }
                });
        }
    }

    menu.addItem({ "New Save", MenuItemType::Action, [this, fileIndex]() {
        // create a new save
        std::string s = "save_" + std::to_string(fileIndex + 1);
        game.eventManager.pushEvent(SAVE_GAME, std::any(s));
        game.eventManager.pushEvent(SELECT_MENU_DONE);
        game.stopScene(getName());
        } });
    // go back to the start menu
    menu.addItem({ "Back", MenuItemType::Action, [&]() {
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
