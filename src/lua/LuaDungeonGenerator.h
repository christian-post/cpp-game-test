#pragma once

#include <sol.hpp>
#include <string>

class Game;

class LuaDungeonGenerator
{
public:
    LuaDungeonGenerator(Game& game);

    void executeScript(const std::string& scriptPath);

private:
    Game& game;
    sol::state lua;

    void setupBindings();
};