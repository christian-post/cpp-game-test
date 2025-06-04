#pragma once
#include "Scene.h"
#include <vector>
#include <string> 
#include <map>
#include <array>
#include <iostream>
#include <functional>
#include "ItemData.h"

class Game;
using InventoryItem = std::pair<const ItemData*, uint32_t>;

class InventoryManager {
public:
    InventoryManager(Game& game);
    const std::array<std::map<std::string, InventoryItem>, NUM_ITEM_TYPES>& getItems() const {
        return items;
    }
    std::map<std::string, ItemData>& getItemData() {
        return itemData;
    }
    uint32_t getItemQuantity(const std::string& key) const {
        for (const auto& typeItems : items) {
            auto it = typeItems.find(key);
            if (it != typeItems.end()) return it->second.second;
        }
        return 0;
    }

private:
    Game& game;
    std::map<std::string, ItemData> itemData; // blueprints for each item
    std::array<std::map<std::string, InventoryItem>, NUM_ITEM_TYPES> items; // the actual inventory
    // private methods for adding and removing items
    // public access is done via event listeners
    void addItem(const std::string& name, uint32_t amount);
    void removeItem(const std::string& name, uint32_t amount);
};