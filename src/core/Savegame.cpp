#include "Savegame.h"
#include "Game.h"

nlohmann::json writeDataToJSON(const SaveGame& saveGame)
{
    nlohmann::json jsonOutput;
    jsonOutput["playerMaxHealth"] = saveGame.playerMaxHealth;
    jsonOutput["playerHealth"] = saveGame.playerHealth;
    jsonOutput["currentWeapons"] = saveGame.currentWeapons;
    jsonOutput["spritesFollowingPlayer"] = saveGame.spritesFollowingPlayer;

    for (const auto& itemPair : saveGame.items)
    {
        jsonOutput["items"].push_back({
            {"key", itemPair.first},
            {"amount", itemPair.second}
            });
    }

    for (size_t lvl = 0; lvl < saveGame.DungeonRooms.size(); lvl++)
    {
        for (const auto& roomEntry : saveGame.DungeonRooms[lvl])
        {
            size_t roomHash = roomEntry.first;
            const RoomData& roomData = roomEntry.second;

            nlohmann::json roomJson;
            roomJson["visited"] = roomData.visited;
            roomJson["dark"] = roomData.dark;
            roomJson["doors"] = roomData.doors;
            roomJson["tilemapKey"] = roomData.tilemapKey;
            roomJson["state"] = roomData.state;

            for (const auto& objectEntry : roomData.objectStates)
            {
                uint32_t objectId = objectEntry.first;
                const ObjectState& objectState = objectEntry.second;
                roomJson["objectStates"][std::to_string(objectId)] = objectState;
            }

            jsonOutput["DungeonLevels"][lvl][std::to_string(roomHash)] = roomJson;
        }
    }

    jsonOutput["DungeonWidth"] = saveGame.dungeonWidth;
    jsonOutput["DungeonHeight"] = saveGame.dungeonHeight;
    jsonOutput["StartingRoomIndex"] = saveGame.startingRoomIndex;
    jsonOutput["StartingLevel"] = saveGame.startingLevel;

    return jsonOutput;
}

SaveGame readSaveDataFromJSON(const nlohmann::json& jsonInput)
{
    SaveGame saveGame;
    saveGame.playerMaxHealth = jsonInput.at("playerMaxHealth").get<uint32_t>();
    saveGame.playerHealth = jsonInput.at("playerHealth").get<uint32_t>();
    saveGame.currentWeapons = jsonInput.at("currentWeapons").get<std::array<std::string, 2>>();
    if (jsonInput.contains("spritesFollowingPlayer"))
    {
        for (const auto& spriteName : jsonInput.at("spritesFollowingPlayer").get<std::vector<std::string>>())
        {
            saveGame.spritesFollowingPlayer.emplace_back(spriteName);
        }
    }

    if (jsonInput.contains("items"))
    {
        for (const auto& itemJson : jsonInput.at("items"))
        {
            std::string key = itemJson.at("key").get<std::string>();
            uint32_t amount = itemJson.at("amount").get<uint32_t>();
            saveGame.items.emplace_back(key, amount);
        }
    }

    // Dungeon metadata
    saveGame.dungeonWidth = jsonInput.at("DungeonWidth").get<size_t>();
    saveGame.dungeonHeight = jsonInput.at("DungeonHeight").get<size_t>();
    saveGame.startingRoomIndex = jsonInput.at("StartingRoomIndex").get<size_t>();
    saveGame.startingLevel = jsonInput.at("StartingLevel").get<size_t>();

    // Dungeon room data
    if (!jsonInput.contains("DungeonLevels"))
    {
        TraceLog(LOG_ERROR, "No data for DungeonLevels found in savegame.");
        return saveGame;
    }

    size_t numLevels = jsonInput["DungeonLevels"].size();
    saveGame.DungeonRooms.resize(numLevels);
    for (size_t lvl = 0; lvl < numLevels; ++lvl)
    {
        const auto& levelData = jsonInput["DungeonLevels"][lvl];
    
        for (auto& [roomHash, roomJson] : levelData.items())
        {
            size_t roomIndex = static_cast<size_t>(std::stoul(roomHash));
            RoomData roomData;
            roomData.visited = roomJson.at("visited").get<bool>();
            roomData.dark = roomJson.at("dark").get<bool>();
            roomData.doors = roomJson.at("doors").get<uint8_t>();
            roomData.state = roomJson.at("state").get<uint8_t>();
            roomData.tilemapKey = roomJson.at("tilemapKey").get<std::string>();

            if (roomJson.contains("objectStates"))
            {
                for (const auto& objectEntry : roomJson.at("objectStates").items())
                {
                    uint32_t objectId = static_cast<uint32_t>(std::stoul(objectEntry.key()));
                    roomData.objectStates[objectId] = objectEntry.value().get<ObjectState>();
                }
            }
            saveGame.DungeonRooms[lvl][roomIndex] = roomData;
        }
    }
    return saveGame;
}

void saveDungeon(SaveGame& saveGame, Dungeon& dungeon)
{
    // writes the necessary data to the savegame object
    // rooms is a vector of optionals and may contain empty entries
    // saveGame.DungeonRooms is a tightly packed hash map (though I could also use null in the JSON)
    //auto& rooms = dungeon.getRooms();
    size_t numLevels = dungeon.getNumLevels();
    saveGame.DungeonRooms.resize(numLevels);
    auto [width, height] = dungeon.getSize();

    for (size_t lvl = 0; lvl < numLevels; lvl++)
    {
        for (size_t i = 0; i < width * height; i++)
        {
            Room* room = dungeon.getRoomAt(lvl, i);
            if (!room)
                continue;

            RoomData rd;
            rd.objectStates = {}; // ensures "null" in JSON for rooms without stateful objects
            // I would make these the same struct, but I don't think that the whole TileMap should be serialized
            // maybe the Tilemap shouldn't be a part of Room; rather just the key, idk
            rd.dark = room->dark;
            rd.doors = room->doors;
            rd.state = room->state;
            rd.objectStates = room->objectStates;
            rd.tilemapKey = room->tilemap.getName();
            rd.visited = room->visited;
            saveGame.DungeonRooms[lvl][i] = rd;
        }
    }
    saveGame.dungeonWidth = width;
    saveGame.dungeonHeight = height;
    saveGame.startingRoomIndex = dungeon.getStartingRoomIndex();
    saveGame.startingLevel = 0; // TODO ist just always 0 right now
}

std::unique_ptr<Dungeon> loadDungeon(SaveGame& saveGame, Game& game)
{
    // creates a dungeon from the save data
    std::unique_ptr dungeon = std::make_unique<Dungeon>(game, saveGame.dungeonWidth, saveGame.dungeonHeight, 1); // TODO levels
    size_t numLevels = saveGame.DungeonRooms.size();

    TraceLog(LOG_INFO, "Loading rooms on level %d", numLevels);
    for (size_t lvl = 0; lvl < numLevels; ++lvl)
    {
        const auto& levelRooms = saveGame.DungeonRooms[lvl];

        for (const auto& [index, roomData] : levelRooms)
        {
            Room room{ game.loader.getTilemap(roomData.tilemapKey), roomData.doors };
            room.dark = roomData.dark;
            room.state = roomData.state;
            room.visited = roomData.visited;
            for (auto& [objID, state] : roomData.objectStates)
            {
                room.objectStates[objID] = state;
            }
            size_t row = index / saveGame.dungeonWidth;
            size_t col = index % saveGame.dungeonWidth;
            dungeon->insertRoom(lvl, row, col, std::move(room));

            TraceLog(LOG_INFO, "Loading Room with index %d (%s) in state %d", index, roomData.tilemapKey.c_str(), room.state);
        }
    }
     
    dungeon->setStartingRoomIndex(saveGame.startingRoomIndex);
    dungeon->setLevel(0);  // TODO
    dungeon->makeMinimapTextures();

    return dungeon;
}
