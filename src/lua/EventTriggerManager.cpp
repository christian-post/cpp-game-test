#include "EventTriggerManager.h"
#include "Game.h"
#include "InGame.h"
#include "TileMap.h"
#include "LuaEventManager.h"
#include "raylib.h"
#include <fstream>

EventTriggerManager::EventTriggerManager(Game& game)
    : game(game)
{
}

void EventTriggerManager::loadTriggers(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        TraceLog(LOG_ERROR, "Failed to load trigger data from %s", filepath.c_str());
        return;
    }

    nlohmann::json data;
    file >> data;

    if (!data.contains("triggers"))
    {
        TraceLog(LOG_WARNING, "No \"triggers\" array found in %s", filepath.c_str());
        return;
    }

    for (const auto& triggerData : data["triggers"])
    {
        EventTrigger trigger;
        trigger.id = triggerData["id"];
        trigger.scriptPath = triggerData["script"];
        trigger.conditions = triggerData["conditions"];
        trigger.hasTriggered = false;

        triggers.push_back(trigger);
    }

    TraceLog(LOG_INFO, "Loaded %d event triggers from: %s", triggers.size(), filepath.c_str());
}

bool EventTrigger::checkConditions(Game& game) const
{
    if (hasTriggered)
        return false;

    const TileMap* tm = game.currentWorld->getCurrentTileMap();
    if (!tm)
        return false;

    for (auto& [key, value] : conditions.items())
    {
        if (key == "roomID" && tm->getRoomID() != value.get<std::string>())
            return false;

        if (key == "roomName" && tm->getName() != value.get<std::string>())
            return false;

        if (key == "roomState" && game.currentWorld->getCurrentRoom()->state != value.get<int>())
            // match exact room state
            return false;

        if (key == "maxRoomState" && game.currentWorld->getCurrentRoom()->state > value.get<int>())
            // triggers up to a certain room state
            return false;

        if (key == "minRoomState" && game.currentWorld->getCurrentRoom()->state < value.get<int>())
            // triggers only in this state and above
            return false;

        if (key == "hasItem" && game.inventory.getItemQuantity(value.get<std::string>()) <= 0)
            return false;

        if (key == "noEnemies" && value.get<bool>())
        {
            bool hasEnemies = std::any_of(game.sprites.begin(), game.sprites.end(),
                [](const std::shared_ptr<Sprite>& s) { 
                    return s->isEnemy; 
                });

            if (hasEnemies)
                return false;
        }
    }

    return true;
}

void EventTriggerManager::update()
{
    for (auto& trigger : triggers)
    {
        if (trigger.checkConditions(game))
        {
            trigger.hasTriggered = true;
            TraceLog(LOG_INFO, "Lua event trigger fired: %s", trigger.id.c_str());

            // add the trigger context
            TriggerContext context;
            context.scriptPath = trigger.scriptPath;
            context.triggerID = trigger.id;
            context.roomID = game.currentWorld->getCurrentTileMap()->getRoomID(); //TODO might throw an error

            game.eventManager.pushEvent(EXECUTE_LUA_CUTSCENE, context);
        }
    }
}