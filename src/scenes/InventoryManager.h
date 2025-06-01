#pragma once
#include "Scene.h"
#include <vector>
#include <string> 
#include <unordered_map>
#include <array>
#include <iostream>
#include <functional>

enum itemType {
    CONSUMABLE,
    WEAPON,
    NUM_ITEM_TYPES // used for size
};

struct Item {
    std::string name;
    uint32_t quantity;
    std::string textureKey;
    std::function<bool()> onConsume = nullptr; 

    Item(const std::string& n, uint32_t q, const std::string& t, std::function<bool()> callback = nullptr)
        : name(n), quantity(q), textureKey(t), onConsume(std::move(callback)) {
    }
};

struct ItemData {
    itemType type;
    std::string textureKey;
    std::function<bool()> onConsume = nullptr; // Callback for item effect, has to return whether 1 qty of that item should be removed from inventory
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
    std::unordered_map<std::string, ItemData> itemData;
    std::array<std::vector<Item>, NUM_ITEM_TYPES> items;
    // private methods for adding and removing items
    // public access is done via event listeners
    void addItem(const std::string& name, uint32_t amount);
    // TODO removeItem
};