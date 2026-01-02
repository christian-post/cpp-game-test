#pragma once
#include "Behavior.h"
#include "WeaponBehavior.h"
#include <string>
#include <map>
#include <functional>
#include <optional>

class Game;

enum ItemType
{
    CONSUMABLE,
    WEAPON,
    PASSIVE,
    KEY,
    IMMEDIATE,
    NUM_ITEM_TYPES // last type used for array size
};

struct ItemData
{
    ItemType type;
    std::string displayName;
    std::string textureKey;
    std::function<bool()> onConsume = nullptr;
    // Weapon-specific data (optional - only used for weapons)
    std::optional<weaponData> weaponBehavior = std::nullopt;
};

std::map<std::string, ItemData> createItemData(Game& game);
