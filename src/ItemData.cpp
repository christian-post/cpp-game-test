#include "ItemData.h"
#include "Game.h"


static weaponData createWeaponDataFromJSON(const nlohmann::json& weaponJSON, const std::string& weaponKey) {
    // Helper function to create weaponData from JSON
    // Get the JSON data with fallback
    const auto& data = weaponJSON.contains(weaponKey)
        ? weaponJSON.at(weaponKey)
        : weaponJSON.at("weapon_default");

    if (!weaponJSON.contains(weaponKey)) {
        TraceLog(LOG_WARNING, "Missing weapon data for %s, falling back to weapon_default", weaponKey.c_str());
    }

    // Build weaponData from JSON
    weaponData wpnData = {};
    wpnData.type = static_cast<weaponType>(data.at("type"));
    wpnData.lifetime = data.at("lifetime");
    wpnData.damage = data.at("damage");
    wpnData.posOffsetX = data.at("posOffsetX");
    wpnData.posOffsetY = data.at("posOffsetY");
    wpnData.HurtboxOffsetX = data.at("HurtboxOffsetX");
    wpnData.HurtboxOffsetY = data.at("HurtboxOffsetY");
    wpnData.HurtboxWidth = data.at("HurtboxWidth");
    wpnData.HurtboxHeight = data.at("HurtboxHeight");

    return wpnData;
}


std::map<std::string, ItemData> createItemData(Game& game) {
    std::map<std::string, ItemData> data;

    data["weapon_sword"] = ItemData{ WEAPON, "Sword", "weapon_sword" };
    data["weapon_double_axe"] = ItemData{ WEAPON, "Double Axe", "weapon_double_axe" };
    data["weapon_bow"] = ItemData{ WEAPON, "Bow", "weapon_bow" };
    data["weapon_hammer"] = ItemData{ WEAPON, "Hammer", "weapon_hammer" };
    data["weapon_mace"] = ItemData{ WEAPON, "Mace", "weapon_mace" };
    data["weapon_spear"] = ItemData{ WEAPON, "Spear", "weapon_spear" };
    data["weapon_baton_with_spikes"] = ItemData{ WEAPON, "Spiked Baton", "weapon_baton_with_spikes" };
    data["item_lamp"] = ItemData{ WEAPON, "Lamp", "item_lamp" },
    data["red_potion"] = ItemData{ CONSUMABLE, "Red Potion", "flask_big_red" };
    data["green_potion"] = ItemData{ CONSUMABLE, "Green Potion", "flask_big_green" };
    data["blue_potion"] = ItemData{ CONSUMABLE, "Blue Potion", "flask_big_blue" };
    data["bomb"] = ItemData{ CONSUMABLE, "Bomb", "bomb" };
    data["coin"] = ItemData{ PASSIVE, "Coin", "itemDropCoin" };
    data["weapon_arrow"] = ItemData{ PASSIVE, "Arrows", "weapon_arrow" };
    data["key"] = ItemData{ PASSIVE, "Key", "item_key" };
    data["heart_drop"] = ItemData{ IMMEDIATE, "Heart", "itemDropHeart" };
    data["heart_1up"] = ItemData{ CONSUMABLE, "Heart 1UP", "itemDropHeart1Up" };

    // consumables callbacks
    data["red_potion"].onConsume = [&game]() {
        static bool isRefilling = false;
        if (isRefilling) 
            return false;

        auto* player = game.getPlayer();
        if (player->health == player->maxHealth) 
            return false;

        int repeats = player->maxHealth - player->health;
        isRefilling = true;
        game.eventManager.pushRepeatedEvent(UNNAMED, 0.2f, {}, [&game]() {
            game.getPlayer()->health += 1;
            game.playSound("heart");
            }, repeats, []() {
                isRefilling = false;
                });
            return true;
        };

    data["heart_1up"].onConsume = [&game]() {
        static bool isRefilling = false;
        if (isRefilling) 
            return false;

        auto* player = game.getPlayer();
        player->maxHealth += 2;
        int repeats = player->maxHealth - player->health;
        isRefilling = true;
        game.eventManager.pushRepeatedEvent(UNNAMED, 0.2f, {}, [&game]() {
            game.getPlayer()->health += 1;
            game.playSound("heart");
            }, repeats, []() {
                isRefilling = false;
                });
            return true;
        };

    data["heart_drop"].onConsume = [&game]() {
        auto* player = game.getPlayer();
        player->health = std::min(player->health + 2, player->maxHealth);
        game.playSound("heart");
        return true;
        };

    // weapon-specific data
    const auto& weaponJSON = game.loader.getSpriteData();

    for (auto& [key, item] : data) {
        if (item.type == WEAPON) {
            // Create weaponData from JSON
            weaponData wpnData = createWeaponDataFromJSON(weaponJSON, key);

            // Add weapon-specific callbacks
            if (key == "item_lamp") {
                wpnData.onCreate = [&game]() {
                    game.eventManager.pushEvent(LAMP_ON);
                    };
                wpnData.onDestroy = [&game]() {
                    game.eventManager.pushEvent(LAMP_OFF);
                    };
            }
            // TOTO: Add other weapon-specific callbacks here as needed...

            // Attach the weaponData to the existing ItemData
            item.weaponBehavior = wpnData;
        }
    }

    return data;
}
