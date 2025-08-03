#pragma once
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include "Dungeon.h"
#include "json.hpp"

struct RoomData {
    // persistent data for a single room
    // see Dungeon.h
    bool visited;
    bool dark;
    uint8_t doors;
    std::unordered_map< uint32_t, ObjectState> objectStates;
    std::string tilemapKey;
};

struct SaveGame {
    // all the data that should be saved between games
    uint32_t playerHealth = 3;
    uint32_t playerMaxHealth = 3;
    std::vector<std::pair<std::string, uint32_t>> items; // <key, amount>; strings correspond to keys in ItemData.cpp
    // TODO: create a data structure that allows for multiple dungeons
    size_t dungeonWidth = 0;
    size_t dungeonHeight = 0;
    size_t startingRoomIndex = 0;
    std::unordered_map<uint32_t, RoomData> DungeonRooms; // hash corresponds to room index
};

nlohmann::json writeDataToJSON(const SaveGame& saveGame);
SaveGame readSaveDataFromJSON(const nlohmann::json& jsonInput);
void saveDungeon(SaveGame & saveGame, Dungeon & dungeon);
std::unique_ptr<Dungeon> loadDungeon(SaveGame& saveGame, Game& game);