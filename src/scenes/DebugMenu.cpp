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
    menus[static_cast<size_t>(MenuType::Main)]->addItem({ "Test Menu", MenuItemType::Action, [&]() {
            activeMenu = MenuType::TestMenu;
        }
        });
    // go back to the InGame Scene from the main menu
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
    // Test Action type
    menus[static_cast<size_t>(MenuType::TestMenu)]->addItem({ "Action Test", MenuItemType::Action, [&]() {
            TraceLog(LOG_INFO, "Action button pressed!");
        }
        });

    // Test Cycle type
    MenuItem cycleItem;
    cycleItem.displayName = "Sound Mode";
    cycleItem.type = MenuItemType::Cycle;
    cycleItem.options = { "Off", "Low", "Medium", "High" };
    cycleItem.currentOption = 2;
    cycleItem.cycleCallback = [&](size_t index) {
        TraceLog(LOG_INFO, "Sound mode changed to index: %zu", index);
        };
    menus[static_cast<size_t>(MenuType::TestMenu)]->addItem(cycleItem);

    // Test another Cycle type
    MenuItem difficultyItem;
    difficultyItem.displayName = "Difficulty";
    difficultyItem.type = MenuItemType::Cycle;
    difficultyItem.options = { "Easy", "Normal", "Hard", "Expert" };
    difficultyItem.currentOption = 1;
    difficultyItem.cycleCallback = [&](size_t index) {
        TraceLog(LOG_INFO, "Difficulty changed to index: %zu", index);
        };
    menus[static_cast<size_t>(MenuType::TestMenu)]->addItem(difficultyItem);

    // Test Number type
    MenuItem volumeItem;
    volumeItem.displayName = "Volume";
    volumeItem.type = MenuItemType::Number;
    volumeItem.numberValue = 50;
    volumeItem.minValue = 0;
    volumeItem.maxValue = 100;
    volumeItem.step = 5;
    volumeItem.numberCallback = [&](int value) {
        TraceLog(LOG_INFO, "Volume changed to: %d", value);
        };
    menus[static_cast<size_t>(MenuType::TestMenu)]->addItem(volumeItem);

    // Test another Number type with different range
    MenuItem speedItem;
    speedItem.displayName = "Game Speed";
    speedItem.type = MenuItemType::Number;
    speedItem.numberValue = 100;
    speedItem.minValue = 50;
    speedItem.maxValue = 200;
    speedItem.step = 10;
    speedItem.numberCallback = [&](int value) {
        TraceLog(LOG_INFO, "Game speed changed to: %d", value);
        };
    menus[static_cast<size_t>(MenuType::TestMenu)]->addItem(speedItem);

    menus[static_cast<size_t>(MenuType::TestMenu)]->addItem({ "Back", MenuItemType::Action, [&]() {
            activeMenu = MenuType::Main;
        }
        });
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