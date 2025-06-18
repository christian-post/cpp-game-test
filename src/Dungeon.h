#pragma once
#include <memory>
#include <vector>
#include <optional>
#include <unordered_map>
#include "TileMap.h"
#include <raylib.h>

class Game;

struct ObjectState {
	// used to store object state between room visits
	bool isOpened = false;
	bool isDefeated = false;
	// tbc
};


class Room {
public:
	uint8_t doors;
	TileMap tilemap;
	uint8_t state = 1;
	bool visited = false;
	std::unordered_map<uint32_t, ObjectState> objectStates;

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
	std::vector<std::optional<Room>> rooms;
public:
	Dungeon(Game& game, size_t roomsW, size_t roomsH);
	size_t getCurrentRoomIndex() const { return currentRoomIndex; }
	void setCurrentRoomIndex(size_t idx) { currentRoomIndex = idx; }
	void advanceRoomState();
	uint8_t getCurrentRoomState();
	std::unordered_map<uint32_t, ObjectState>& getCurrentRoomObjectStates();
	const TileMap* loadCurrentTileMap();
	void insertRoom(size_t row, size_t col, Room&& room);
	std::pair<size_t, size_t> getSize() const;
	std::pair<size_t, size_t> getRoomSize(size_t index) const;
	bool hasVisited(size_t index) const;
	void setVisited(size_t index) { rooms[index]->visited = true; }
	std::vector<RenderTexture2D> minimapTextures;
	void makeMinimapTextures();
};