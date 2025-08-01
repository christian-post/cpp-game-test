#include "Savegame.h"

nlohmann::json writeDataToJSON(const SaveGame& saveGame)
{
    nlohmann::json jsonOutput;
    jsonOutput["playerMaxHealth"] = saveGame.playerMaxHealth;
    jsonOutput["playerHealth"] = saveGame.playerHealth;

    for (const auto& itemPair : saveGame.items) {
        jsonOutput["items"].push_back({
            {"key", itemPair.first},
            {"amount", itemPair.second}
            });
    }

    for (const auto& roomEntry : saveGame.DungeonRooms) {
        uint32_t roomHash = roomEntry.first;
        const RoomData& roomData = roomEntry.second;

        nlohmann::json roomJson;
        roomJson["visited"] = roomData.visited;
        roomJson["dark"] = roomData.dark;
        roomJson["doors"] = roomData.doors;
        roomJson["tilemapKey"] = roomData.tilemapKey;

        for (const auto& objectEntry : roomData.objectStates) {
            uint32_t objectId = objectEntry.first;
            const ObjectState& objectState = objectEntry.second;
            roomJson["objectStates"][std::to_string(objectId)] = objectState;
        }

        jsonOutput["DungeonRooms"][std::to_string(roomHash)] = roomJson;
    }

    return jsonOutput;
}

SaveGame readSaveDataFromJSON(const nlohmann::json& jsonInput)
{
    SaveGame saveGame;
    saveGame.playerMaxHealth = jsonInput.at("playerMaxHealth").get<uint32_t>();
    saveGame.playerHealth = jsonInput.at("playerHealth").get<uint32_t>();

    if (jsonInput.contains("items")) {
        for (const auto& itemJson : jsonInput.at("items")) {
            std::string key = itemJson.at("key").get<std::string>();
            uint32_t amount = itemJson.at("amount").get<uint32_t>();
            saveGame.items.emplace_back(key, amount);
        }
    }

    if (jsonInput.contains("DungeonRooms")) {
        for (const auto& roomEntry : jsonInput.at("DungeonRooms").items()) {
            uint32_t roomHash = static_cast<uint32_t>(std::stoul(roomEntry.key()));
            const auto& roomJson = roomEntry.value();

            RoomData roomData;
            roomData.visited = roomJson.at("visited").get<bool>();
            roomData.dark = roomJson.at("dark").get<bool>();
            roomData.doors = roomJson.at("doors").get<uint8_t>();
            roomData.tilemapKey = roomJson.at("tilemapKey").get<std::string>();

            if (!roomJson.contains("objectStates"))
                continue;
            for (const auto& objectEntry : roomJson.at("objectStates").items()) {
                uint32_t objectId = static_cast<uint32_t>(std::stoul(objectEntry.key()));
                roomData.objectStates[objectId] = objectEntry.value().get<ObjectState>();
            }

            saveGame.DungeonRooms[roomHash] = roomData;
        }
    }

    return saveGame;
}

void saveDungeon(SaveGame& saveGame, Dungeon& dungeon)
{
    // writes the necessary data to the savegame object
    // rooms is a vector of optionals and may contain empty entries
    // saveGame.DungeonRooms is a tightly packed hash map (though I could also use null in the JSON)
    auto rooms = dungeon.getRooms();
    for (uint32_t i = 0; i < rooms.size(); i++) {
        if (rooms[i]) {
            RoomData rd;
            rd.objectStates = {}; // ensures "null" in JSON for rooms without stateful objects
            // I would make these the same struct, but I don't think that the whole TileMap should be serialized
            // maybe the Tilemap shouldn't be a part of Room; rather just the key, idk
            rd.dark = rooms[i]->dark;
            rd.doors = rooms[i]->doors;
            rd.objectStates = rooms[i]->objectStates;
            rd.tilemapKey = rooms[i]->tilemap.getTilesetName();
            rd.visited = rooms[i]->visited;
            saveGame.DungeonRooms[i] = rd;
        }
    }
}
