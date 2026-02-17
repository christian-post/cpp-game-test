#include "DungeonMenu.h"
#include "Controls.h"
#include "Utils.h"
#include "LuaDungeonGenerator.h"
#include <string>
#include <utility>
#include <ctime>
#include <sstream>
#include <iomanip>

std::string seedToHexString(uint32_t seed)
{
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setw(8) << std::setfill('0') << seed;
    return ss.str();
}

DungeonMenu::DungeonMenu(Game& game, const std::string& name)
	: Scene(game, name), menu(MenuSelect(game))
{
    game.eventManager.addListener(DUNGEON_GENERATION_START, [this](std::any) {
        currentState = MenuState::Generating;
        });

    game.eventManager.addListener(DUNGEON_GENERATION_PROGRESS, [this](std::any message) {
        displayMessage = std::any_cast<std::string>(message);
        });

    game.eventManager.addListener(DUNGEON_GENERATION_COMPLETE, [this](std::any success) {
        if (std::any_cast<bool>(success))
        {
            currentState = MenuState::Idle;
        }
        else
        {
            displayMessage = "Failed to generate a valid dungeon.\nPress any key to retry.";
            currentState = MenuState::Failed;
        }
        });
}

void DungeonMenu::startup()
{
    if (game.getSetting<bool>("useRngSeed"))
    {
        // TODO change to menu where the players can enter a seed
        uint32_t seed = game.getSetting<int>("dungeonRngSeed");
        std::string seedMenuText = format("Generate with seed: (%s)", seedToHexString(seed).c_str());

        menu.addItem({
            std::move(seedMenuText),
            MenuItemType::Action,
            [&, seed]() {
                TraceLog(LOG_INFO, "Using dungeon RNG seed: %d", seed);
                game.luaDungeonGen->setSeed(seed);

                TraceLog(LOG_INFO, "Running Lua dungeon generation test");
                game.luaDungeonGen->executeScript("scripts/test_dungeon_generation.lua");
                TraceLog(LOG_INFO, "Dungeon generation completed!");

                reloadRequired = true;
            }
            });
    }

    menu.addItem({
            "Generate at random",
            MenuItemType::Action,
            [&]() {
                // get seed from system time
                uint32_t seed = static_cast<uint32_t>(time(nullptr));

                TraceLog(LOG_INFO, "Using dungeon RNG seed: %d", seed);
                game.luaDungeonGen->setSeed(seed);

                TraceLog(LOG_INFO, "Running Lua dungeon generation test");
                game.luaDungeonGen->executeScript("scripts/test_dungeon_generation.lua");
                TraceLog(LOG_INFO, "Dungeon generation completed! Seed: %d", seed);

                std::string seedStr = seedToHexString(seed);
                lastSeedMessage = "Seed of generated dungeon: " + seedStr;

                reloadRequired = true;
            }
        });

    // go back to start menu
    menu.addItem({
        "Back",
        MenuItemType::Action,
        [&]() {
            game.startScene("StartMenu");
            game.stopScene(getName());
        }
        });
}

void DungeonMenu::update(float deltaTime)
{
    if (currentState == MenuState::Idle)
    {
        menu.update();
    }
    else if (currentState == MenuState::Failed)
    {
        if (AnyKeyPressed(game.buttonsPressed))
            currentState = MenuState::Idle;
    }
}

void DungeonMenu::draw()
{
    int fontSize = 10;

    if (currentState == MenuState::Generating or currentState == MenuState::Failed)
    {
        ClearBackground(BLACK);

        const char* text = displayMessage.c_str();
        int margin = 24;
        // put the text at the bottom right
        int textWidth = MeasureText(text, fontSize);
        int x = game.gameScreenWidth - textWidth - margin;
        int y = game.gameScreenHeight - fontSize - margin;
        DrawText(text, x, y, fontSize, LIGHTGRAY);
    }
    else
    {
        menu.draw();

        if (lastSeedMessage.size())
        {
            const char* text = lastSeedMessage.c_str();
            int marginBottom = 24;
            // put the text at the mid bottom
            int textWidth = MeasureText(text, fontSize);
            int x = game.gameScreenWidth / 2 - textWidth / 2;
            int y = game.gameScreenHeight - fontSize - marginBottom;
            DrawText(text, x, y, fontSize, LIGHTGRAY);
        }
    }
}

void DungeonMenu::end()
{
    // reload some of the files that got modified during dungeon generation
    if (reloadRequired)
    {
        // JSON
        game.loader.loadQueue.emplace("Loading JSON files", [&]() {
            game.loader.loadSpriteData("./resources/npcs.json");
            game.loader.loadtextData("./resources/texts.json");
            game.loader.loadDungeonData("./resources/dungeons.json");
            });

        // rooms
        game.loader.loadQueue.emplace("Loading tilemaps", [&]() {
            game.loader.loadTilemapsFromDirectory("./resources/tilemaps/generated/lua_dungeon"); // TODO load subfolders recursively
            });

        // TODO will this take time long enough so I need to show a loading message?
        while (!game.loader.loadQueue.empty())
        {
            game.loader.loadQueue.front().second(); // execute callback
            game.loader.loadQueue.pop();
        }
    }

    reloadRequired = false;
}
