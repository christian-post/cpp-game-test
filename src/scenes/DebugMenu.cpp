#include "DebugMenu.h"
#include "Game.h"
#include "raylib.h"
#include <functional>
#include <string>
#include <map>
#include <any>
#include <tuple>


DebugMenu::DebugMenu(Game& game, const std::string& name) : Scene(game, name) {
    // initialize all menus
    for (size_t i = 0; i < numSubMenus; i++) {
        menus[i] = std::make_unique<MenuSelect>(game);
        menus[i]->setFontSize(6);
        menus[i]->setYMargin(12);
    }
}

void DebugMenu::startup()
{
    // configure main menu
    menus[0]->addItem({ "Room Select", [&]() {
            // change to submenu
            activeMenu = 1;
        }
        });
    menus[0]->addItem({ "Item Cheat", [&]() {
            activeMenu = 2;
        }
        });
    // go back to the InGame Scene from the main menu
    menus[0]->addItem({ "Back to Game", [&]() {
            game.eventManager.pushEvent(SELECT_MENU_DONE);
            game.stopScene(getName());
        } 
        });

    // submenu 1 (room selection)
    const std::pair<size_t, size_t> size = game.currentDungeon->getSize();
    for (size_t i = 0; i < size.first * size.second; i++) {
        Room* room = game.currentDungeon->getRoomAt(i);
        if (room) {
            menus[1]->addItem({ std::to_string(i) + " : " + room->tilemap.getName(), [&, i]() {
                    // change to this room
                    game.currentDungeon->setCurrentRoomIndex(i);
                    game.eventManager.pushEvent(RELOAD_ROOM);
                    game.eventManager.pushEvent(SELECT_MENU_DONE);
                    game.stopScene(getName());
                }
                });
        }
    }

    menus[1]->addItem({ "Back", [&]() {
            activeMenu = 0;
        }
        });

    // submenu 2 (adding items)
    auto& itemData = game.inventory.getItemData();
    for (const auto& [key, item] : itemData) {
        uint32_t qty = game.inventory.getItemQuantity(key);
        menus[2]->addItem({ key + " x " + std::to_string(qty), [this, key]() {
            game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(key, 1));
        }
        });
    }

    menus[2]->addItem({ "Back", [&]() {
            activeMenu = 0;
        }
        });
}

void DebugMenu::update(float deltaTime)
{
    menus[activeMenu]->update();

    // update the displayed item quantities
    if (activeMenu == 2) {
        auto& itemData = game.inventory.getItemData();
        size_t index = 0;
        for (const auto& [key, item] : itemData) {
            uint32_t qty = game.inventory.getItemQuantity(key);
            menus[2]->updateItemText(index, key + " x " + std::to_string(qty));
            index++;
        }
    }
}

void DebugMenu::draw()
{
    menus[activeMenu]->draw();
}

void DebugMenu::end()
{
}
