#include "OpenLockBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "InGame.h"
#include "Commands.h"
#include "Controls.h"
#include <any>
#include <tuple>
#include <utility>

OpenLockBehavior::OpenLockBehavior(Game& game, std::shared_ptr<Sprite> door, std::shared_ptr<Sprite> player, const int triggerKey, std::string itemName)
    : game{ game }, door{ door }, player{ player }, triggerKey{ triggerKey }, itemName{ std::move(itemName) }
{}

void OpenLockBehavior::update(float deltaTime)
{
    if (triggered)
        return;

    if (auto d = door.lock(), p = player.lock(); d && p)
    {
        if (d->isMarkedForDeletion())
            return;

        const float padding = 2.0f;
        interactionRect.x = d->rect.x - padding;
        interactionRect.y = d->rect.y - padding;
        interactionRect.width = d->rect.width + 2.0f * padding;
        interactionRect.height = d->rect.height + 2.0f * padding;
        if (CheckCollisionRecs(interactionRect, p->rect))
        {
            if (!collided)
            {
                int button = game.getGamepadButtonForControl(CONTROL_ACTION1);
                game.eventManager.pushEvent(SHOW_HELP_TEXT, std::make_any<std::tuple<std::string, char, int>>(std::tuple<std::string, char, int>{"OPEN", 'O', button}));
                collided = true;
            }

            if (game.buttonsDown & CONTROL_ACTION1)
            {
                triggered = true;
                // itemName = key type (normal key, boss key, etc)
                uint32_t qty = game.inventory.getItemQuantity(itemName);
                if (qty == 0)
                {
                    game.cutsceneManager.queueCommand(new Command_Textbox(game, "Looks like you need a key to open this door."));
                    game.cutsceneManager.queueCommand(new Command_Callback([this]() {
                        game.eventManager.pushDelayedEvent(UNNAMED, 0.2f, nullptr, [this]() {
                            triggered = false;
                            });
                        }));
                    return;
                }
                // remove the item
                game.eventManager.pushEvent(REMOVE_ITEM, std::make_any<std::pair<std::string, uint32_t>>(itemName, 1));
                game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [d, this]() {
                    this->game.playSound("bookPlace1");
                    //d->currentFrame = 0;
                    // TODO opening animation?
                    this->game.eventManager.pushEvent(triggerKey);
                    });
                game.eventManager.pushDelayedEvent(UNNAMED, 0.8f, nullptr, [d, this]() {
                    this->game.playSound("doorOpen_2");
                    //d->currentFrame = 1;
                    d->visible = false;
                    d->staticCollision = false;
                    this->done = true;
                    });
            }
        }
        else
        {
            if (collided)
            {
                game.eventManager.pushEvent(HIDE_HELP_TEXT);
                collided = false;
            }
        }
    }
}

void OpenLockBehavior::reset()
{
    Behavior::reset();
    triggered = false;
    collided = true;
}
