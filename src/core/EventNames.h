#pragma once
#include <string>
#include <unordered_map>

// Event key aliases to make the code more maintainable
// TODO: add a comment to each event describing what it does and how it's used

enum EventName
{
    SAVE_GAME,
    LOAD_GAME,
    ADD_ITEM,
    REMOVE_ITEM,
    CONSUME_ITEM,
    HIDE_HUD,
    SHOW_HUD,
    WEAPON_SET,
    ITEM_ADDED,
    SHOW_COIN_AMOUNT,
    HIDE_COIN_AMOUNT,
    SHOW_HELP_TEXT,
    HIDE_HELP_TEXT,
    MOVE_CAMERA,
    TELEPORT, // change the world, level, room index, player position, and reload the room
    RELOAD_ROOM, // just reload the room
    SET_MUSIC_VOLUME,
    SCREEN_SHAKE, // data: tuple[duration, xMag, yMag]
    KILL_WEAPON, // weapon sprite is done
    INVENTORY_DONE,
    SELECT_MENU_DONE,
    LOADING_SAVEGAME_SUCCESS,
    LAMP_ON, // player activates the lamp
    LAMP_OFF, // player deactivates the lamp
    LOCK_PLAYER_MOVEMENT, // the user can't steer the player sprite
    UNLOCK_PLAYER_MOVEMENT,
    INTERMEDIATE_LOADING_FINISHED, // signals that loading during the game has finished TODO unused
    CAMERA_UPDATE, // dispatches once per frame when the camera updates; contains the camera reference and player world position
    DUNGEON_GENERATION_START,
    DUNGEON_GENERATION_TICK,
    DUNGEON_GENERATION_COMPLETE,
    UNNAMED, // used whenever no name is needed (delayed or repeated events that have no listeners)
    STATIC_EVENT_COUNT // needs to be at the last position
};

// for debugging only
inline const char* EventNameStrings[] = {
    "SAVE_GAME",
    "LOAD_GAME",
    "ADD_ITEM",
    "REMOVE_ITEM",
    "CONSUME_ITEM",
    "HIDE_HUD",
    "SHOW_HUD",
    "WEAPON_SET",
    "ITEM_ADDED",
    "SHOW_COIN_AMOUNT",
    "HIDE_COIN_AMOUNT",
    "SHOW_HELP_TEXT",
    "HIDE_HELP_TEXT",
    "MOVE_CAMERA",
    "TELEPORT",
    "RELOAD_ROOM",
    "SET_MUSIC_VOLUME",
    "SCREEN_SHAKE",
    "KILL_WEAPON",
    "INVENTORY_DONE",
    "SELECT_MENU_DONE",
    "LOADING_SAVEGAME_SUCCESS",
    "LAMP_ON",
    "LAMP_OFF",
    "LOCK_PLAYER_MOVEMENT",
    "UNLOCK_PLAYER_MOVEMENT",
    "INTERMEDIATE_LOADING_FINISHED",
    "CAMERA_UPDATE",
    "DUNGEON_GENERATION_START",
    "DUNGEON_GENERATION_TICK",
    "DUNGEON_GENERATION_COMPLETE",
    "UNNAMED"
};

inline const char* GetEventNameString(EventName event) {
    if (event >= 0 && event < STATIC_EVENT_COUNT) {
        return EventNameStrings[event];
    }
    // if the event name exceeds the hard-coded events, it is probably a custom event
    static char buffer[32];
    snprintf(buffer, sizeof(buffer), "CUSTOM_EVENT(%d)", static_cast<int>(event));
    return buffer;
}


class EventKeyRegistry
{
    // allows for event strings (from JSON data) to be registered as EventNames
    // TODO: call it either "EventName" or "EventKey" consistently...
    // TODO: idk if I can avoid using strings altogether...
public:
    static int getEventKey(const std::string& name)
    {
        auto& map = nameToId();
        auto it = map.find(name);
        if (it != map.end())
            return it->second;

        int id = nextId++;
        map[name] = id;
        return id;
    }

    static int getIndexedEventKey(EventName base, int index)
    {
        std::string key = std::to_string(base) + "_" + std::to_string(index);
        auto& map = nameToId();
        auto it = map.find(key);
        if (it != map.end())
            return it->second;

        int id = nextId++;
        map[key] = id;
        return id;
    }

    static int getNewKey()
    {
        return nextId++;
    }
private:
    static std::unordered_map<std::string, int>& nameToId()
    {
        static std::unordered_map<std::string, int> map;
        return map;
    }

    static inline int nextId = STATIC_EVENT_COUNT;
};