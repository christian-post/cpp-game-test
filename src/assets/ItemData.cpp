#include "ItemData.h"
#include "Game.h"
#include "WeaponBehavior.h"


static weaponData createWeaponDataFromJSON(const nlohmann::json& weaponJSON, const std::string& weaponKey)
{
    // Helper function to create weaponData from JSON
    // Get the JSON data with fallback
    const auto& data = weaponJSON.contains(weaponKey)
        ? weaponJSON.at(weaponKey)
        : weaponJSON.at("weapon_default");

    if (!weaponJSON.contains(weaponKey))
    {
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
    wpnData.soundKey = data.at("sound");

    if (data.at("needsAmmo"))
    {
        wpnData.needsAmmo = true;
        wpnData.ammoType = data.at("ammoType");
    }

    // projectiles
    if (wpnData.type == SHOOT || wpnData.type == BOW || wpnData.type == HOOKSHOT)
    {
        wpnData.projectileKey = data.at("projectile");
        wpnData.projectileTrailEmitterKey = data.at("projectileTrailEmitter");
        wpnData.projectileImpactEmitterKey = data.at("projectileImpactEmitter");
    }

    if (wpnData.type == HOOKSHOT)
    {
        wpnData.maxHookshotRange = data.value("maxHookshotRange", 80.0f);
        wpnData.hookshotPullSpeed = data.value("hookshotPullSpeed", 160.0f);
    }

    return wpnData;
}


std::map<std::string, ItemData> createItemData(Game& game)
{
    std::map<std::string, ItemData> data;

    data["weapon_sword"] = ItemData{ WEAPON, "weapon_sword", "Sword", "weapon_sword" };
    data["weapon_double_axe"] = ItemData{ WEAPON, "weapon_double_axe", "Double Axe", "weapon_double_axe" };
    data["weapon_bow"] = ItemData{ WEAPON, "weapon_bow", "Bow", "weapon_bow" };
    data["weapon_hookshot"] = ItemData{ WEAPON, "weapon_hookshot", "Hookshot", "weapon_hookshot" };
    data["weapon_hammer"] = ItemData{ WEAPON, "weapon_hammer", "Hammer", "weapon_hammer" };
    data["weapon_mace"] = ItemData{ WEAPON, "weapon_mace", "Mace", "weapon_mace" };
    data["weapon_spear"] = ItemData{ WEAPON, "weapon_spear", "Spear", "weapon_spear" };
    data["weapon_baton_with_spikes"] = ItemData{ WEAPON, "weapon_baton_with_spikes", "Spiked Baton", "weapon_baton_with_spikes" };
    data["weapon_bomb"] = ItemData{ WEAPON, "weapon_bomb", "Bomb", "bomb" };
    data["item_lamp"] = ItemData{ WEAPON, "item_lamp", "Lamp", "item_lamp" };

    data["red_potion"] = ItemData{ CONSUMABLE, "red_potion", "Red Potion", "flask_big_red" };
    data["green_potion"] = ItemData{ CONSUMABLE, "green_potion", "Green Potion", "flask_big_green" };
    data["blue_potion"] = ItemData{ CONSUMABLE, "blue_potion", "Blue Potion", "flask_big_blue" };
    data["heart_1up"] = ItemData{ CONSUMABLE, "heart_1up", "Heart 1UP", "heart_1up" };

    data["bombs"] = ItemData{ PASSIVE, "bombs", "Bombs", "bomb" }; // TODO use different sprite
    data["arrows"] = ItemData{ PASSIVE, "arrows", "Arrows", "weapon_arrow" };
    data["key"] = ItemData{ PASSIVE, "key", "Key", "item_key" };
    data["boss_key"] = ItemData{ PASSIVE, "boss_key", "Boss Key", "item_boss_key" };
    data["coin"] = ItemData{ PASSIVE, "coin", "Coin", "itemDropCoin" };

    data["heart_drop"] = ItemData{ IMMEDIATE, "heart_drop", "Heart", "itemDropHeart" };

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

    for (auto& [key, item] : data)
    {
        if (item.type == WEAPON)
        {
            // Create weaponData from JSON
            weaponData wpnData = createWeaponDataFromJSON(weaponJSON, key);

            // Add weapon-specific callbacks
            if (key == "item_lamp")
            {
                wpnData.onCreate = [&game]() {
                    game.eventManager.pushEvent(LAMP_ON);
                    };
                wpnData.onDestroy = [&game]() {
                    game.eventManager.pushEvent(LAMP_OFF);
                    };
            }
            else if (key == "weapon_bomb")
            {
                wpnData.onCreate = [&game]() {
                    game.eventManager.pushEvent(THROW_BOMB);
                    };
            }
            // TOTO: Add other weapon-specific callbacks here as needed...
            // Or maybe organize them differently

            // Attach the weaponData to the existing ItemData
            item.weaponBehavior = wpnData;
        }
    }

    return data;
}
