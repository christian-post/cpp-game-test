#pragma once

#include <sol.hpp>
#include <string>

class Game;
class InGame;

class LuaEventManager
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