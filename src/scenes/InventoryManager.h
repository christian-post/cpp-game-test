#pragma once
#include "Scene.h"
#include <vector>
#include <string> 
#include <unordered_map>
#include <array>
#include <iostream>

enum itemType {
    CONSUMABLE,
    WEAPON,
    NUM_ITEM_TYPES // used for size
};

struct Item {
    std::string name;
    uint32_t quantity;
    std::string textureKey;
    Item(const std::string& n, uint32_t q, const std::string& t)
        : name(n), quantity(q), textureKey(t) {
    }
};

class InventoryManager : public Scene {
public:
    InventoryManager(Game& game, const std::string& name) : Scene(game, name) {}
    void startup() override;
    void update(float dt) override;
    void draw() override;
    void end() override;
    std::array<std::vector<Item>, NUM_ITEM_TYPES>& getItems() { return items; }

private:
    // TODO refactor itemData at some point (json?)
    // pairs are itemType, textureKey
    std::unordered_map<std::string, std::pair<itemType, std::string>> itemData;
    std::array<std::vector<Item>, NUM_ITEM_TYPES> items;
    // private methods for adding and removing items
    // public access is done via event listeners
    void addItem(const std::string& name, uint32_t amount);
};