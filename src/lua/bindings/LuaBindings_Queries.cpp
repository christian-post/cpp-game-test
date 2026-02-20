#include "LuaBindings.h"
#include "Game.h"
#include "InGame.h"
#include "TileMap.h"

void bindGameQueries(sol::state& lua, Game& game, InGame& inGame)
{
    lua.set_function("spriteExists", [&](const std::string& name) {
        return game.spriteMap.find(name) != game.spriteMap.end();
        });

    lua.set_function("getRoomState", [&]() {
        return game.currentWorld->getCurrentRoom()->state;
        });

    lua.set_function("hasItem", [&](const std::string& item) {
        return game.inventory.getItemQuantity(item) > 0;
        });

    lua.set_function("getCurrentRoomID", [&]() -> std::string {
        const TileMap* tm = game.currentWorld->getCurrentTileMap();
        return tm ? tm->getRoomID() : "";
        });

    lua.set_function("getCurrentRoomName", [&]() -> std::string {
        const TileMap* tm = game.currentWorld->getCurrentTileMap();
        return tm ? tm->getName() : "";
        });

    lua.set_function("getTileSize", [&]() {
        return inGame.tilemapRenderer.getTileSize();
        });

    lua.set_function("getScreenWidth", [&]() {
        return game.gameScreenWidth;
        });

    lua.set_function("getScreenHeight", [&]() {
        return game.gameScreenHeight;
        });

    lua.set_function("getSetting", [&](std::string key) -> float {
        return game.getSetting<float>(key);
        });

    lua.set_function("getPlayerPosition", [&lua]() -> sol::table {
        // Note: you'll need to add this - getting player position
        // For now returning a dummy table structure
        auto table = lua.create_table();
        table["x"] = 0.0f;
        table["y"] = 0.0f;
        return table;
        });
}