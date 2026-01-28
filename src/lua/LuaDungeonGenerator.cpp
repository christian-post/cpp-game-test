#include "LuaDungeonGenerator.h"
#include "Game.h"


LuaDungeonGenerator::LuaDungeonGenerator(Game& game)
    : game(game)
{
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::package, sol::lib::table, sol::lib::os);
    lua["package"]["path"] = lua["package"]["path"].get<std::string>() + ";scripts/?.lua";

    setupBindings();
}

void LuaDungeonGenerator::setupBindings()
{
    // empty for now - add dungeon bindings later
}