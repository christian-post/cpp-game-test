#include "Game.h"
#include "DebugMenu.h"
#include <functional>
#include <string>
#include <map>
#include <any>
#include <tuple>
#include <cstdint>

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
    menus[0]->addItem({ "Level Select", [&]() {
            // change to submenu
            activeMenu = 2;
        }
        });
    menus[0]->addItem({ "Item Cheat", [&]() {
            activeMenu = 3;
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
        size_t level = game.currentDungeon->getCurrentLevel();
        Room* room = game.currentDungeon->getRoomAt(level, i);
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

    // submenu 2 (level select)
    size_t numLevels = game.currentDungeon->getNumLevels();
    for (size_t i = 0; i < numLevels; i++) 
    {
        menus[2]->addItem({"Level: " + std::to_string(i), [&, i]() {
            // change to this room
            game.currentDungeon->setLevel(i);
            game.eventManager.pushEvent(RELOAD_ROOM);
            game.eventManager.pushEvent(SELECT_MENU_DONE);
            game.stopScene(getName());
            }
            });
    }

    menus[2]->addItem({ "Back", [&]() {
            activeMenu = 0;
        }
        });

    // submenu 3 (adding items)
    auto& itemData = game.inventory.getItemData();
    for (const auto& [key, item] : itemData) {
        uint32_t qty = game.inventory.getItemQuantity(key);
        menus[3]->addItem({ key + " x " + std::to_string(qty), [this, key]() {
            game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(key, 1));
        }
        });
    }

    menus[3]->addItem({ "Back", [&]() {
            activeMenu = 0;
        }
        });
}

void DebugMenu::update(float deltaTime)
{
    menus[activeMenu]->update();

    // update the displayed item quantities
    if (activeMenu == 3) {
        auto& itemData = game.inventory.getItemData();
        size_t index = 0;
        for (const auto& [key, item] : itemData) {
            uint32_t qty = game.inventory.getItemQuantity(key);
            menus[3]->updateItemText(index, key + " x " + std::to_string(qty));
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
