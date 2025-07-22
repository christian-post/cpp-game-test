#pragma once
#include <string>
#include <map>
#include <functional>

enum ItemType {
    CONSUMABLE,
    WEAPON,
    PASSIVE,
    KEY,
    IMMEDIATE,
    NUM_ITEM_TYPES // last type used for array size
};

struct ItemData {
    //std::string key;
    ItemType type;
    std::string displayName;
    std::string textureKey;
    std::function<bool()> onConsume = nullptr;
};

std::map<std::string, ItemData> createItemData();
