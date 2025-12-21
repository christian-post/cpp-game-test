#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <raylib.h>
#include "json.hpp"
#include "TileMap.h"

class Game;
class WorldGraph;

struct ObjectState {
    // used to store object state between room visits
    bool isOpened = false;
    bool isDefeated = false;
    size_t dialogIndex = 0;
    // TODO: tbc
};

// overloaded json loading and serialization functions
inline void to_json(nlohmann::json& jsonOutput, const ObjectState& state) {
    jsonOutput = {
        { "isOpened", state.isOpened },
        { "isDefeated", state.isDefeated },
        { "dialogIndex", state.dialogIndex }
    };
}

inline void from_json(const nlohmann::json& jsonInput, ObjectState& state) {
    jsonInput.at("isOpened").get_to(state.isOpened);
    jsonInput.at("isDefeated").get_to(state.isDefeated);
    jsonInput.at("dialogIndex").get_to(state.dialogIndex);
}


class Room {
public:
    uint8_t doors; // 4-bit mask with one bit for each cardinal direction, starting at the right and going counter clockwise
    TileMap tilemap;
    uint8_t state = 1;
    bool visited = false;
    bool dark = false; // in dark rooms, the player needs a lamp
    std::unordered_map<uint32_t, ObjectState> objectStates; // makes object states (dead etc) persistent

    Room(TileMap tilemap, uint8_t doors = 0b0000)
        : doors(doors), tilemap(std::move(tilemap)), dark(this->tilemap.isDark())
    {}
};

class Level {
    // Encapsulates a single floor of a dungeon, which is a grid of Room objects.
    // A dungeon can have any number of floors
public:
    Level(size_t roomsW, size_t roomsH);
    std::vector<std::optional<Room>>& getRooms(); // reference to all rooms
    Room* getRoomAt(size_t index); // pointer to a specific room
    void insertRoom(size_t index, Room&& room);

private:
    size_t roomsW;
    size_t roomsH;
    std::vector<std::optional<Room>> rooms;
};

class Dungeon {
private:
    Game& game;
    size_t roomsW;
    size_t roomsH;
    size_t currentLevel = 0;
    size_t currentRoomIndex = 0;
    size_t startingRoomIndex = 0;
    bool playerHasBeenPlaced = false;
    std::vector<Level> levels;

public:
    Dungeon(Game& game, size_t roomsW, size_t roomsH, size_t numLevels);
    std::vector<std::optional<Room>>& getRooms();  // returns a Rooms reference (currentLevel by default)
    std::vector<std::optional<Room>>& getRooms(size_t level);  // overload for a given level index
    Room* getRoomAt(size_t level, size_t index);

    void setStartingRoomIndex(size_t index); // defines in which room the player starts;
    void setCurrentRoomIndex(size_t index) { currentRoomIndex = index; } // defines which room (on the grid) the player is currently in
    size_t getCurrentRoomIndex() const { return currentRoomIndex; }
    size_t getStartingRoomIndex() const { return startingRoomIndex; }
    size_t getCurrentLevel() const { return currentLevel; }

    void advanceRoomState(); // advances the current room's state
    void advanceRoomState(size_t level, size_t index); // advances the specified room's state
    uint8_t getCurrentRoomState();

    bool isRoomDark();
    uint8_t getRoomDoors(size_t level, size_t index);
    std::unordered_map<uint32_t, ObjectState>& getCurrentRoomObjectStates();
    std::unordered_map<uint32_t, ObjectState>& getRoomObjectStates(size_t level, size_t index);
    const TileMap* loadTileMap(); // TODO: does this "load" or "get"? should it be renamed?
    const TileMap* loadTileMap(size_t level, size_t index);
    void insertRoom(size_t level, size_t row, size_t col, Room&& room);
    std::pair<size_t, size_t> getSize() const;  // gets ( rooms wide, rooms high )
    std::pair<size_t, size_t> getRoomSize(size_t level, size_t index); // gets the width and height of the room in pixels
    bool hasVisited(size_t level, size_t index);
    void setVisited(size_t level, size_t index);
    void makeMinimapTextures();
    std::vector<std::vector<RenderTexture2D>> minimapTextures; // TODO should each level store its textures instead?
    WorldGraph buildGraphFromDungeon(
        const std::string& start,
        const std::vector<std::tuple<std::string, std::string, std::vector<std::string>>>& edges,
        const std::unordered_set<std::string>& itemNodes
    ); // TODO strings should be changed to indices later
    void generate(const std::string& dungeonKey);
};