#include "ItemData.h"
#include "Game.h"


std::map<std::string, ItemData> createItemData() {
    std::map<std::string, ItemData> data;

    data["weapon_sword"] = ItemData{ WEAPON, "Sword", "weapon_sword" };
    data["weapon_double_axe"] = ItemData{ WEAPON, "Double Axe", "weapon_double_axe" };
    data["weapon_arrow"] = ItemData{ CONSUMABLE, "Arrows", "weapon_arrow" };
    data["red_potion"] = ItemData{ CONSUMABLE, "Red Potion", "flask_big_red_idle" };
    data["coin"] = ItemData{ PASSIVE, "Coin", "itemDropCoin_idle" };

    return data;
}
