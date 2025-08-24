#include "raylib.h"
#include "LoadSavegameMenu.h"
#include "Game.h"
#include "Utils.h"

LoadSavegameMenu::LoadSavegameMenu(Game& game, const std::string& name)
    : Scene(game, name), menu(MenuSelect(game)) {

    menu.setFontSize(8);
    menu.setMargin(6);

    game.eventManager.addListener(LOADING_SAVEGAME_SUCCESS, [&](const std::any& data) {
        game.startScene("InGame");
        game.startScene("HUD");
        game.stopScene(getName());
        });
}

void LoadSavegameMenu::startup()
{
    std::vector<std::string> files = listFiles("./savegames");
    if (files.size() == 0) {
        // TODO: this should not be necessary to check, but whatever
        menu.addItem({
            "No files", nullptr
            });
        return;
    }
    for (const auto& file : files) {
        // extract the basename to display as the menu item
        // TODO: strip the ".json" also
        size_t pos = file.find("savegames") + 10;
        std::string substr = file.substr(pos);
        TraceLog(LOG_INFO, "%s", substr.c_str());
        menu.addItem({ substr, [this, substr]() {
            game.eventManager.pushEvent(LOAD_GAME, std::any(substr));
            }
            });
    }
    // go back to the start menu
    menu.addItem({ "Back", [&]() {
        game.startScene("StartMenu");
        game.stopScene(getName());
        } });
}

void LoadSavegameMenu::update(float deltaTime)
{
    menu.update();
}

void LoadSavegameMenu::draw()
{
    menu.draw();
}

void LoadSavegameMenu::end()
{
}
