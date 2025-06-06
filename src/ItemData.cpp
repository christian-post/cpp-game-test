#include "ItemData.h"
#include "Game.h"


std::map<std::string, ItemData> createItemData() {
    std::map<std::string, ItemData> data;

    data["weapon_sword"] = ItemData{ WEAPON, "Sword", "weapon_sword" };
    data["weapon_double_axe"] = ItemData{ WEAPON, "Double Axe", "weapon_double_axe" };
    data["weapon_bow"] = ItemData{ WEAPON, "Bow", "weapon_bow" };
    data["weapon_hammer"] = ItemData{ WEAPON, "Hammer", "weapon_hammer" };
    data["weapon_mace"] = ItemData{ WEAPON, "Mace", "weapon_mace" };
    data["weapon_spear"] = ItemData{ WEAPON, "Spear", "weapon_spear" };
    data["weapon_baton_with_spikes"] = ItemData{ WEAPON, "Spiked Baton", "weapon_baton_with_spikes" };
    data["red_potion"] = ItemData{ CONSUMABLE, "Red Potion", "flask_big_red_idle" };
    data["green_potion"] = ItemData{ CONSUMABLE, "Green Potion", "flask_big_green_idle" };
    data["blue_potion"] = ItemData{ CONSUMABLE, "Blue Potion", "flask_big_blue_idle" };
    data["bomb"] = ItemData{ CONSUMABLE, "Bomb", "bomb_idle" };
    data["coin"] = ItemData{ PASSIVE, "Coin", "itemDropCoin_idle" };
    data["weapon_arrow"] = ItemData{ PASSIVE, "Arrows", "weapon_arrow" };

    return data;
}
