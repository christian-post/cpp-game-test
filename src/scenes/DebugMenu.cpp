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
    menus[static_cast<size_t>(MenuType::Main)]->addItem({ "Room Select", MenuItemType::Action, [&]() {
            activeMenu = MenuType::RoomSelect;
        }
        });
    menus[static_cast<size_t>(MenuType::Main)]->addItem({ "Level Select", MenuItemType::Action, [&]() {
            activeMenu = MenuType::LevelSelect;
        }
        });
    menus[static_cast<size_t>(MenuType::Main)]->addItem({ "Item Cheat", MenuItemType::Action, [&]() {
            activeMenu = MenuType::ItemCheat;
        }
        });
    MenuItem noClip;
    noClip.displayName = "No Clip";
    noClip.type = MenuItemType::Cycle;
    noClip.options = { "Off", "On" };
    noClip.currentOption = game.getPlayer()->isColliding ? 0 : 1;
    noClip.cycleCallback = [&](size_t index) {
        Sprite* player = game.getPlayer();
        if (!player)
            return;

        player->isColliding = !player->isColliding;
        if (player->iFrameTimer == 0)
            player->iFrameTimer = FLT_MAX; // basically infinite
        else
            player->iFrameTimer = 0;
        };
    menus[static_cast<size_t>(MenuType::Main)]->addItem(noClip);

    menus[static_cast<size_t>(MenuType::Main)]->addItem({ "Back to Game", MenuItemType::Action, [&]() {
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
            menus[static_cast<size_t>(MenuType::RoomSelect)]->addItem({ std::to_string(i) + " : " + room->tilemap.getName(), MenuItemType::Action, [&, i]() {
                // change to this room
                game.currentDungeon->setCurrentRoomIndex(i);
                game.eventManager.pushEvent(RELOAD_ROOM);
                game.eventManager.pushEvent(SELECT_MENU_DONE);
                game.stopScene(getName());
            }
                });
        }
    }

    menus[static_cast<size_t>(MenuType::RoomSelect)]->addItem({ "Back", MenuItemType::Action, [&]() {
            activeMenu = MenuType::Main;
        }
        });

    // submenu 2 (level select)
    size_t numLevels = game.currentDungeon->getNumLevels();
    for (size_t i = 0; i < numLevels; i++)
    {
        menus[static_cast<size_t>(MenuType::LevelSelect)]->addItem({ "Level: " + std::to_string(i), MenuItemType::Action, [&, i]() {
            // change to this room
            game.currentDungeon->setLevel(i);
            game.eventManager.pushEvent(RELOAD_ROOM);
            game.eventManager.pushEvent(SELECT_MENU_DONE);
            game.stopScene(getName());
            }
            });
    }

    menus[static_cast<size_t>(MenuType::LevelSelect)]->addItem({ "Back", MenuItemType::Action, [&]() {
            activeMenu = MenuType::Main;
        }
        });

    // submenu 3 (adding items)
    auto& itemData = game.inventory.getItemData();
    for (const auto& [key, item] : itemData) {
        uint32_t qty = game.inventory.getItemQuantity(key);
        menus[static_cast<size_t>(MenuType::ItemCheat)]->addItem({ key + " x " + std::to_string(qty), MenuItemType::Action, [this, key]() {
            game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(key, 1));
        }
            });
    }

    menus[static_cast<size_t>(MenuType::ItemCheat)]->addItem({ "Back", MenuItemType::Action, [&]() {
            activeMenu = MenuType::Main;
        }
        });

    // submenu 4 (test menu for new item types)

}

void DebugMenu::update(float deltaTime)
{
    menus[menuIndex()]->update();

    // update the displayed item quantities
    if (activeMenu == MenuType::ItemCheat)
    {
        auto& itemData = game.inventory.getItemData();
        size_t index = 0;
        for (const auto& [key, item] : itemData) {
            uint32_t qty = game.inventory.getItemQuantity(key);
            menus[static_cast<size_t>(MenuType::ItemCheat)]->updateItemText(index, key + " x " + std::to_string(qty));
            index++;
        }
    }
}

void DebugMenu::draw()
{
    menus[menuIndex()]->draw();
}

void DebugMenu::end()
{
}