#include "LuaEventManager.h"
#include "bindings/LuaBindings.h"
#include "Game.h"
#include "InGame.h"
#include "raylib.h"
#include "LuaDungeonGenerator.h"
#include "EventTriggerManager.h"

LuaEventManager::LuaEventManager(Game& game, InGame& inGame)
    : game(game), inGame(inGame)
{
    lua.open_libraries(sol::lib::base, sol::lib::package, sol::lib::string, sol::lib::math);
    lua["package"]["path"] = lua["package"]["path"].get<std::string>() + ";scripts/?.lua";

    setupBindings();
}

void LuaEventManager::executeEvent(const TriggerContext& context)
{
    auto result = lua.safe_script_file(context.scriptPath);
    if (!result.valid())
    {
        sol::error err = result;
        TraceLog(LOG_ERROR, "Lua script load error: %s", err.what());
        return;
    }

    // make context data accesible to lua
    lua["triggerID"] = context.triggerID;
    lua["roomID"] = context.roomID;

    sol::protected_function execute = lua["execute"];
    if (execute.valid())
    {
        auto exec_result = execute();
        if (!exec_result.valid())
        {
            sol::error err = exec_result;
            TraceLog(LOG_ERROR, "Lua execute error: %s", err.what());
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Script %s has no execute() function", context.scriptPath.c_str());
    }
}

void LuaEventManager::setupBindings()
{
    bindCutsceneCommands(lua, game, inGame);
    bindGameQueries(lua, game, inGame);
    bindSpriteOperations(lua, game, inGame);
}