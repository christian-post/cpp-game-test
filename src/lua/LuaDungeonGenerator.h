#pragma once

#include <sol.hpp>
#include "raylib.h"
#include "json.hpp"
#include <string>

class Game;

class LuaDungeonGenerator
{
public:
    LuaDungeonGenerator(Game& game);

    void setSeed(int seed);

    template<typename... Args>
    void executeScript(const std::string& scriptPath, Args&&... args)
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
            auto exec_result = execute(std::forward<Args>(args)...);
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

private:
    Game& game;
    sol::state lua;

    void setupBindings();
    nlohmann::json solToJson(const sol::object& obj); // converts a lua table to json
    sol::object jsonToSol(const nlohmann::json& j); // other way round
};