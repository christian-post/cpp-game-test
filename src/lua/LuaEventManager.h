#pragma once

#include <sol.hpp>
#include <string>

class Game;
class InGame;

class LuaEventManager
    // processes Lua script that control events (cutscenes etc) in the InGame scene
{
public:
    LuaEventManager(Game& game, InGame& inGame);

    void executeEvent(const std::string& scriptPath);

private:
    Game& game;
    InGame& inGame;
    sol::state lua;

    void setupBindings();
};