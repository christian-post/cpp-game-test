#include "raylib.h"
#include "LoadSavegameMenu.h"
#include "Game.h"
#include "Utils.h"
#include <vector>

LoadSavegameMenu::LoadSavegameMenu(Game& game, const std::string& name)
    : Scene(game, name), menu(MenuSelect(game)) {
    // menu config
    menu.setFontSize(8);
    menu.setYMargin(6);
    menu.setXMargin(30);
    menu.setPosition(MenuPosition::LEFT);
    // event that transitions to the InGame scene once this scene is done
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

    for (auto& entry : fs::directory_iterator("./savegames")) {
        if (entry.path().extension() == ".json") {
            // extract the basename to display as the menu item
            std::string substr = entry.path().stem().string();
            menu.addItem({ substr, [this, substr]() {
                game.eventManager.pushEvent(LOAD_GAME, std::any(substr));
                }
                });
            // get information about the file (helps with identifying the save)
            fileInfo.emplace_back(GetLastWriteTime(entry.path().string()));
        }
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

    size_t idx = menu.getCurrentIndex();
    if (idx < fileInfo.size()) {
        std::string& infoStr = fileInfo[idx];
        // TODO place text depending on the size of the string
        int x = static_cast<int>(game.gameScreenWidth * 0.4);
        DrawText(infoStr.c_str(), x, 20, 8, LIGHTGRAY);
        // draw the thumbnail (0th element)
        auto& thumbnail = game.loader.getTextures(menu.getCurrentItem().displayName);
        if (thumbnail.size())
            DrawTexture(thumbnail[0], x, 40, WHITE);
    }
}
