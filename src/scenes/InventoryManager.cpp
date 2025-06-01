#include "InventoryManager.h"
#include "Game.h"
#include "raylib.h"

void InventoryManager::startup() {
    // TODO: this gets replaced by an external function or file
    itemData["Sword"] = { WEAPON, "weapon_sword" };
    itemData["Double Axe"] = { WEAPON, "weapon_double_axe" };
    itemData["Arrows"] = { CONSUMABLE, "weapon_arrow" };
    itemData["Red Potion"] = {
        CONSUMABLE,
        "flask_big_red_idle",
        [this]() { 
            // TODO: trigger an event that animates the refilling
            auto* player = game.getPlayer();
            if (player->health == player->maxHealth) 
                return false; // don't consume if player is at max health
            player->refillHealth();
            game.playSound("heal");
            return true;
        }
    };
    itemData["Coin"] = { CONSUMABLE, "itemDropCoin_idle" };

    game.eventManager.addListener("addItem", [this](const std::any& data) {
        if (const auto* item = std::any_cast<std::pair<std::string, uint32_t>>(&data)) {
            addItem(item->first, item->second);
            // TODO: do this for all items
            /*if (item->first == "Coin") 
                this->game.eventManager.pushEvent("coinAdded");*/
        }
        });

    game.eventManager.addListener("consumeItem", [this](const std::any& data) {
        if (const auto* key = std::any_cast<std::string>(&data)) {
            for (Item& item : items[CONSUMABLE]) {
                if (item.textureKey == *key && item.quantity > 0) {
                    bool consume = true;
                    if (item.onConsume) consume = item.onConsume(); // handle callback

                    if (!consume) break; // item was not successfully consumed

                    item.quantity--;
                    if (item.quantity == 0) {
                        // Optionally remove the item if quantity is zero
                        items[CONSUMABLE].erase(
                            std::remove_if(items[CONSUMABLE].begin(), items[CONSUMABLE].end(),
                                [&](const Item& i) { return i.textureKey == *key; }),
                            items[CONSUMABLE].end());
                    }
                    // TODO: push an event specific to the item, or add a callback to a consumable
                    break;
                }
            }
        }
        });
}

void InventoryManager::addItem(const std::string& name, uint32_t amount) {
    auto it = itemData.find(name);
    if (it == itemData.end()) return;

    itemType type = it->second.type;
    const std::string& key = it->second.textureKey;
    auto& callback = it->second.onConsume;

    auto& vec = items[static_cast<size_t>(type)];
    for (Item& i : vec) {
        if (i.name == name) {
            i.quantity += amount;
            return;
        }
    }
    vec.emplace_back(name, amount, key, callback);
}

void InventoryManager::update(float dt) {

}

void InventoryManager::draw() {

}

void InventoryManager::end() {

}