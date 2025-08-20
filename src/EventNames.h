#pragma once
#include <string>
#include <unordered_map>


// TODO: add a comment to each event describing what it does and where it's used

enum EventName {
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
    TELEPORT,
    SET_MUSIC_VOLUME,
    SCREEN_SHAKE,
    KILL_WEAPON,
    INVENTORY_DONE,
    SELECT_MENU_DONE,
    LOADING_SAVEGAME_SUCCESS,
    UNNAMED, // used whenever no name is needed (delayed or repeated events that have no listeners)
    STATIC_EVENT_COUNT // needs to be at the last position
};


class EventKeyRegistry {
    // allows for event strings (from JSON data) to be registered as EventNames
public:
    static int getEventKey(const std::string& name) {
        auto& map = nameToId();
        auto it = map.find(name);
        if (it != map.end())
            return it->second;

        int id = nextId++;
        map[name] = id;
        return id;
    }

private:
    static std::unordered_map<std::string, int>& nameToId() {
        static std::unordered_map<std::string, int> map;
        return map;
    }

    static inline int nextId = STATIC_EVENT_COUNT;
};