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
            { "key", itemPair.first },
            { "amount", itemPair.second }
            });
    }

    jsonOutput["lastWorld"] = saveGame.lastWorld;

    for (const auto& [key, world] : saveGame.worldData)
    {
        size_t lvl = 0;
        for (const auto& roomMap : world)
        {
            for (const auto& [index, roomData] : roomMap)
            {
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

                jsonOutput["worldData"][key][lvl][std::to_string(index)] = roomJson;
            }
            ++lvl;
        }
    }

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

    saveGame.lastWorld = jsonInput["lastWorld"];

    for (auto& [worldName, worldJson] : jsonInput["worldData"].items())
    {
        size_t numLevels = worldJson.size();
        saveGame.worldData[worldName].resize(numLevels);

        for (size_t lvl = 0; lvl < numLevels; ++lvl)
        {
            const auto& levelData = worldJson[lvl];
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

                saveGame.worldData[worldName][lvl][roomIndex] = roomData;
            }
        }
    }

    return saveGame;
}

void saveWorld(SaveGame& saveGame, World& world)
{
    std::string& worldName = world.name;
    size_t numLevels = world.getNumLevels();
    auto [width, height] = world.getSize();

    saveGame.worldData[worldName].resize(numLevels);

    for (size_t lvl = 0; lvl < numLevels; lvl++)
    {
        for (size_t i = 0; i < width * height; i++)
        {
            Room* room = world.getRoomAt(lvl, i);
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
            saveGame.worldData[worldName][lvl][i] = rd;
        }
    }

}

void loadWorld(SaveGame& saveGame, Game& game, std::string& name)
{
    bool isDungeon = (name == "overworld");
    game.createWorld(name, isDungeon);

    auto [width, height] = game.currentWorld->getSize();

    // TODO optimize memory access
    for (size_t lvl = 0; lvl < game.currentWorld->getNumLevels(); ++lvl)
    {
        const auto& levelRooms = saveGame.worldData[name][lvl]; // all rooms on this level

        for (const auto& [index, roomData] : levelRooms)
        {
            Room* room = game.currentWorld->getRoomAt(lvl, index);
            room->dark = roomData.dark;
            room->state = roomData.state;
            room->visited = roomData.visited;
            for (auto& [objID, state] : roomData.objectStates)
            {
                room->objectStates[objID] = state;
            }
            // debugging message
            TraceLog(LOG_INFO, "Loading Room in level %d with index %d (%s) in state %d", lvl, index, roomData.tilemapKey.c_str(), room->state);
        }
    }
}
