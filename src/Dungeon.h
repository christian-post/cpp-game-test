#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <unordered_map>
#include "TileMap.h"
#include <raylib.h>
#include "json.hpp"

class Game;

struct ObjectState {
    // used to store object state between room visits
    bool isOpened = false;
    bool isDefeated = false;
    size_t dialogIndex = 0;
    // tbc
};

// overloaded json loading and serialization functions
inline void to_json(nlohmann::json& jsonOutput, const ObjectState& state) {
    jsonOutput = {
        {"isOpened", state.isOpened},
        {"isDefeated", state.isDefeated},
        {"dialogIndex", state.dialogIndex}
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
        : doors(doors), tilemap(std::move(tilemap)) {
    }
};

class Dungeon {
private:
    Game& game;
    size_t roomsW;
    size_t roomsH;
    size_t currentRoomIndex = 0;
    size_t startingRoomIndex = 0;
    bool playerHasBeenPlaced = false;
    std::vector<std::optional<Room>> rooms;

public:
    Dungeon(Game& game, size_t roomsW, size_t roomsH);
    std::vector<std::optional<Room>>& getRooms();
    void setStartingRoomIndex(size_t idx); // defines in which room the player starts;
    void setCurrentRoomIndex(size_t idx) { currentRoomIndex = idx; } // defines which room (on the grid) the player is currently in
    size_t getCurrentRoomIndex() const { return currentRoomIndex; }
    size_t getStartingRoomIndex() const { return startingRoomIndex; }
    void advanceRoomState(); // advances the current room's state
    void advanceRoomState(size_t index); // advances the specified room's state
    uint8_t getCurrentRoomState();
    bool isRoomDark();
    uint8_t getRoomDoors(size_t index); // TODO: might make a single getter for "Room"...
    std::unordered_map<uint32_t, ObjectState>& getCurrentRoomObjectStates();
    const TileMap* loadCurrentTileMap(); // TODO: does this load or get?
    void insertRoom(size_t row, size_t col, Room&& room);
    std::pair<size_t, size_t> getSize() const;  // gets ( rooms wide, rooms high )
    std::pair<size_t, size_t> getRoomSize(size_t index) const; // gets the width and height of the room in pixels
    bool hasVisited(size_t index) const;
    void setVisited(size_t index);
    std::vector<RenderTexture2D> minimapTextures;
    void makeMinimapTextures();
};