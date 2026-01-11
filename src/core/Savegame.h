#pragma once
#include "Dungeon.h"
#include "World.h"
#include "json.hpp"
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <cstdint>


struct RoomData
{
    // persistent data for a single room
    // see Dungeon.h
    bool visited = false;
    bool dark = false;
    uint8_t doors = 0b0000;
    uint8_t state = 1;
    std::unordered_map< uint32_t, ObjectState> objectStates;
    std::string tilemapKey;
};

struct SaveGame
{
    // all the data that should be saved between games
    uint32_t playerHealth = 3;
    uint32_t playerMaxHealth = 3;
    std::array<std::string, 2> currentWeapons;
    std::vector<std::string> spritesFollowingPlayer; // save keys for sprites that follow the player (might be more than one idk)
    std::vector<std::pair<std::string, uint32_t>> items; // <key, amount>; strings correspond to keys in ItemData.cpp
    std::unordered_map<std::string, std::vector<std::unordered_map<size_t, RoomData>>> worldData; // hierarchy of keys/indices is: world_name > level_index > room_index
};

nlohmann::json writeDataToJSON(const SaveGame& saveGame);
SaveGame readSaveDataFromJSON(const nlohmann::json& jsonInput);
void saveWorld(SaveGame& saveGame, World& world);
void loadWorld(SaveGame& saveGame, Game& game, std::string& name); // loads the given world from the saveGame object as game.currentWorld
