#include "MapUI.h"
#include "Game.h"
#include "Controls.h"
#include "raylib.h"
#include "Utils.h"

MapUI::MapUI(Game& game, const std::string& name) : Scene(game, name) {}

void MapUI::startup() {
    x = float(game.gameScreenWidth); // start the map screen on the right
    topY = game.getSetting("HudHeight");
    y = topY;
    width = game.gameScreenWidth;
    height = game.gameScreenHeight;
    // set the sliding speed so that it takes "slideDuration" seconds to expand the inventory
    speed = height / slideDuration;
    state = SLIDING_LEFT;
}

void MapUI::update(float deltaTime) {
    switch (state) {
    case OPENING:
        if (y > topY) {
            y = std::max(topY, y - deltaTime * speed);
        }
        else {
            state = OPENED;
        }
        break;
    case CLOSING:
        if (y < height) {
            y = std::min(static_cast<float>(height), y + deltaTime * speed);
        }
        else {
            game.eventManager.pushEvent("InventoryDone");
        }
        break;
    }
}

void MapUI::draw() {
    DrawRectangle(int(x), int(y), int(width), int(height), DARKBURGUNDY); // background
}

void MapUI::end() {
    game.eventManager.pushEvent("setMusicVolume", 1.0f);
}