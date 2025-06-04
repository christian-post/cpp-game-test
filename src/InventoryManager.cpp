#include "InventoryManager.h"
#include "Game.h"
#include "ItemData.h"
#include "raylib.h"


InventoryManager::InventoryManager(Game& game) : game(game) {
    itemData = createItemData();
    // TODO: add callbacks back to the items
    itemData["red_potion"].onConsume = [this, isRefilling = false]() mutable {
        if (isRefilling) return false;

        auto* player = this->game.getPlayer();
        if (player->health == player->maxHealth)
            return false;

        int repeats = player->maxHealth - player->health;
        isRefilling = true;
        this->game.eventManager.pushRepeatedEvent("refill_health", 0.2f, {}, [=]() {
            player->health += 1;
            this->game.playSound("heart");
            }, repeats, [&]() {
                isRefilling = false;
                });
            return true;
        };

    game.eventManager.addListener("addItem", [this](const std::any& data) {
        if (const auto* item = std::any_cast<std::pair<std::string, uint32_t>>(&data)) {
            addItem(item->first, item->second);
        }
        });

    game.eventManager.addListener("removeItem", [this](const std::any& data) {
        if (const auto* item = std::any_cast<std::pair<std::string, uint32_t>>(&data)) {
            removeItem(item->first, item->second);
        }
        });

    game.eventManager.addListener("consumeItem", [this](const std::any& data) {
        if (const auto* key = std::any_cast<std::string>(&data)) {
            for (auto it = items[CONSUMABLE].begin(); it != items[CONSUMABLE].end(); ++it) {
                if (it->second.first->textureKey == *key && it->second.second > 0) {
                    if (!it->second.first->onConsume || !it->second.first->onConsume()) break;

                    it->second.second--;
                    if (it->second.second == 0) {
                        items[CONSUMABLE].erase(it);
                    }
                    break;
                }
            }
        }
        });
}

void InventoryManager::addItem(const std::string& key, uint32_t amount) {
    auto it = itemData.find(key);
    if (it == itemData.end()) {
        TraceLog(LOG_ERROR, "No data exists for item with key %s", key.c_str());
        return;
    }

    const ItemData* data = &it->second;
    auto& bucket = items[static_cast<size_t>(data->type)];

    auto& item = bucket[key];
    if (!item.first) item.first = data;
    item.second += amount;
}

void InventoryManager::removeItem(const std::string& key, uint32_t amount) {
    auto it = itemData.find(key);
    if (it == itemData.end()) return;

    const ItemData* data = &it->second;
    auto& bucket = items[static_cast<size_t>(data->type)];

    auto itemIt = bucket.find(key);
    if (itemIt == bucket.end()) return;

    if (itemIt->second.second <= amount) {
        bucket.erase(itemIt);
    }
    else {
        itemIt->second.second -= amount;
    }
}
