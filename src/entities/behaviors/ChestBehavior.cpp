#include <any>
#include <tuple>

#include "ChestBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "InGame.h"
#include "Commands.h"
#include "Controls.h"
#include "ItemData.h"
#include "Utils.h"

ChestBehavior::ChestBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, const std::string& itemName, uint32_t itemAmount)
    : game{ game }, self{ self }, player{ player }, itemName{ itemName }, itemAmount{ itemAmount } {}

void ChestBehavior::update(float deltaTime) {
    if (auto s = self.lock(), p = player.lock(); s && p) {
        interactionRect.x = s->rect.x;
        interactionRect.y = s->rect.y;
        interactionRect.width = s->rect.width;
        interactionRect.height = s->rect.height + 4.0f;
        if (CheckCollisionRecs(interactionRect, p->rect)) {
            if (triggered)
                return;
            if (!collided) {
                game.eventManager.pushEvent(SHOW_HELP_TEXT, std::make_any<std::tuple<std::string, char, int>>(std::tuple<std::string, char, int>{"OPEN", 'O', 9}));
                collided = true;
            }
            if (game.buttonsDown & CONTROL_ACTION1) {
                triggered = true;
                auto& itemData = game.inventory.getItemData();
                const ItemData& data = itemData.at(itemName);  // TODO check for missing key
                s->currentFrame = 2;
                showItem = true;
                game.playSound("doorOpen_2");
                game.eventManager.pushDelayedEvent(UNNAMED, 2.0f, nullptr, [&]() {
                    showItem = false;
                    });

                game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.playSound("Rise03");
                    }));
                game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
                std::string message;
                if (itemAmount == 1) {
                    if (data.type == WEAPON) {
                        message = format("You got the %s.\nOpen your inventory to equip it, then use with [P].", data.displayName.c_str());
                    }
                    else {
                        message = format("You got a %s.", data.displayName.c_str());
                    }
                }
                else {
                    message = format("You got: %s x%u", data.displayName.c_str(), itemAmount);
                }
                game.cutsceneManager.queueCommand(new Command_Textbox(game, message));
                game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(itemName, itemAmount));
                std::string eventStr = "chest_opened_" + std::to_string(s->tileMapID);
                int eventKey = EventKeyRegistry::getEventKey(eventStr);
                game.eventManager.pushEvent(eventKey, s->tileMapID);
            }
        }
        else {
            if (collided) {
                game.eventManager.pushEvent(HIDE_HELP_TEXT);
                collided = false;
            }
        }
    }
}

void ChestBehavior::draw() {
    if (!showItem)
        return;
    if (auto s = self.lock()) {
        int x = (int)s->position.x;
        int y = (int)s->position.y - 16;
        auto& itemData = game.inventory.getItemData();
        const ItemData& data = itemData.at(itemName);
        const auto& textures = game.loader.getTextures(data.textureKey);
        if (textures.size() == 0) {
            TraceLog(LOG_ERROR, "No texture found for %s", data.textureKey.c_str());
            return;
        }
        int item_tex_width = textures[0].width;
        int chest_tex_width = s->frames[s->currentAnimState][s->currentFrame].width;
        x += (chest_tex_width - item_tex_width) / 2;
        DrawTexture(textures[0], x, y, WHITE);
    }
}

void ChestBehavior::reset() {
    Behavior::reset();
    triggered = false;
    collided = false;
    showItem = false;
}
