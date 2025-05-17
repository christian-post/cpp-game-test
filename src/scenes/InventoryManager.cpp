#include "InventoryManager.h"
#include "Game.h"
#include "raylib.h"

void InventoryManager::startup() {
    // TODO: this gets replaced by a JSON file
    itemData["Sword"] = { WEAPON, "weapon_sword" };
    itemData["Double Axe"] = { WEAPON, "weapon_double_axe" };
    itemData["Arrows"] = { CONSUMABLE, "weapon_arrow" };
    itemData["Red Potion"] = { CONSUMABLE, "flask_big_red" };

    game.eventManager.addListener("addItem", [this](const std::any& data) {
        if (const auto* item = std::any_cast<std::pair<std::string, uint32_t>>(&data)) {
            addItem(item->first, item->second);
        }
        });
}

void InventoryManager::addItem(const std::string& name, uint32_t amount) {
    auto it = itemData.find(name);
    if (it == itemData.end()) return;

    itemType type = it->second.first;
    const std::string& key = it->second.second;

    auto& vec = items[static_cast<size_t>(type)];
    for (Item& i : vec) {
        if (i.name == name) {
            i.quantity += amount;
            return;
        }
    }
    vec.emplace_back(name, amount, key);
}

void InventoryManager::update(float dt) {

}

void InventoryManager::draw() {

}

void InventoryManager::end() {

}