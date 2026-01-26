#include "LuaDungeonGenerator.h"
#include "Game.h"
#include "raylib.h"

LuaDungeonGenerator::LuaDungeonGenerator(Game& game)
    : game(game)
{
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::package, sol::lib::table);
    lua["package"]["path"] = lua["package"]["path"].get<std::string>() + ";scripts/?.lua";

    setupBindings();
}

void LuaDungeonGenerator::executeScript(const std::string& scriptPath)
{
    auto result = lua.safe_script_file(scriptPath);
    if (!result.valid())
    {
        sol::error err = result;
        TraceLog(LOG_ERROR, "Lua script error: %s", err.what());
        return;
    }

    sol::protected_function execute = lua["execute"];
    if (execute.valid())
    {
        auto exec_result = execute();
        if (!exec_result.valid())
        {
            sol::error err = exec_result;
            TraceLog(LOG_ERROR, "Lua execute() error: %s", err.what());
            return;
        }
    }
    else
    {
        TraceLog(LOG_WARNING, "Script %s has no execute() function", scriptPath.c_str());
    }

    TraceLog(LOG_INFO, "Lua script executed successfully: %s", scriptPath.c_str());
}

void LuaDungeonGenerator::setupBindings()
{
    // empty for now - add dungeon bindings later
}