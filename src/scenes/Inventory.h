#pragma once
#include "Scene.h"
#include <iostream>

enum inventoryState {
    NONE,
    OPENING,
    OPENED,
    CLOSING
};


struct TestItem {
    // TODO just for testing the inventory
    std::string name;
    uint32_t quantity;
    std::string textureKey;
    TestItem(const std::string& n, uint32_t q, const std::string& t)
        : name(n), quantity(q), textureKey(t) {
    }
};


class Inventory : public Scene {
    // scene responsible for drawing the inventory and all the items
    // as well as item selection etc
    //
    // inventory management is done in Game (I think)
public:
    Inventory(Game& game, const std::string& name) : Scene(game, name) {}
    void startup() override;
    void update(float dt) override;
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

    std::vector<TestItem> weapons;
    std::vector<TestItem> items;
};