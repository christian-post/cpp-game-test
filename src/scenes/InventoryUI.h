#pragma once
#include "Scene.h"
#include <iostream>
#include <cstdint>
#include <vector>
#include <string> 
#include <unordered_map>
#include <array>
#include "ItemData.h"


enum inventoryState {
    NONE,
    OPENING,
    OPENED,
    CLOSING
};

class InventoryUI : public Scene {
    // scene responsible for drawing the inventory and all the items
    // as well as item selection etc
public:
    InventoryUI(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    uint32_t x = 0;
    float topY = 0; // gets adjusted to the HUD height
    float y = 0; // needs to be float for animation precision
    uint32_t height = 0;
    uint32_t width = 0;
    float speed = 0.0f;
    float slideDuration = 1.0f;
    inventoryState state = NONE;
    size_t index = 0; // selected item index
    uint32_t weaponsRows = 3;
    uint32_t cols = 6;
    std::map<std::string, ItemData> itemData;
};
