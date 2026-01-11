#include "World.h"
#include "Game.h"

World::World(Game& game, size_t roomsW, size_t roomsH, size_t numLevels, std::string& name)
    : game{ game }, roomsW{ roomsW }, roomsH{ roomsH }, name{ name }
{
    levels.reserve(numLevels);
    for (size_t i = 0; i < numLevels; ++i)
    {
        levels.emplace_back(roomsW, roomsH);
    }
}

Room* World::getRoomAt(size_t level, size_t index)
{
    if (level >= levels.size()) // TODO is this check sensible here? 
        return nullptr;

    return levels[level].getRoomAt(index);
}

Room* World::getCurrentRoom()
{
    return getRoomAt(currentLevel, currentRoomIndex);
}

const TileMap* World::getTileMap(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        TraceLog(LOG_ERROR, "getTileMap(): No room on level %zu at index %zu", level, index);
        return nullptr;
    }
    return &room->tilemap;
}

const TileMap* World::getCurrentTileMap()
{
    Room* room = getCurrentRoom();
    if (!room)
        return nullptr;

    return &room->tilemap;
}

std::pair<size_t, size_t> World::getSize() const
{
    return { roomsW, roomsH };
}

size_t World::getNumLevels() const
{
    return levels.size();
}

void World::insertRoom(size_t level, size_t row, size_t col, Room&& room)
{
    size_t index = row * roomsW + col;

    // resize levels vector if necessary
    if (level >= levels.size())
    {
        while (level >= levels.size())
        {
            levels.emplace_back(roomsW, roomsH);
        }
    }

    if (!getRoomAt(level, index))
    {
        levels[level].insertRoom(index, std::move(room));
    }
    else
    {
        TraceLog(LOG_WARNING, "insertRoom(): A room already exists on level %zu at index %zu", level, index);
    }
}

void World::setStartingRoomIndex(size_t index)
{
    startingRoomIndex = index;
    if (!playerHasBeenPlaced)
    {
        playerHasBeenPlaced = true;
        currentRoomIndex = index;
    }
}

void World::advanceRoomState()
{
    advanceRoomState(currentLevel, currentRoomIndex);
}

void World::advanceRoomState(size_t level, size_t index)
{
    Room* room = getRoomAt(level, index);
    if (!room)
    {
        TraceLog(LOG_ERROR, "advanceRoomState(): No room on level %zu at index %zu", level, index);
        return;
    }
    // make sure the room state is at least 1
    (room->state <<= 1) || (room->state = 1);
    TraceLog(LOG_INFO, "Room state of %s is now %d", room->tilemap.getName().c_str(), room->state);
}