#include "TradeItemBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "Commands.h"
#include "Controls.h"
#include <any>
#include <algorithm>

TradeItemBehavior::TradeItemBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::string name, uint32_t price)
    : game{ game }, self{ self }, player{ player }, name{ name }, price{ price }
{}

void TradeItemBehavior::update(float deltaTime)
{
    if (triggered)
        return;

    if (auto s = self.lock(), p = player.lock(); s && p)
    {
        if (s->isMarkedForDeletion())
            return;

        if (CheckCollisionRecs(s->rect, p->rect))
        {
            if (!collided)
            {
                game.eventManager.pushEvent(SHOW_COIN_AMOUNT);
                collided = true;
            }
            if (game.buttonsDown & CONTROL_ACTION1)
            {
                triggered = true;
                uint32_t qty = game.inventory.getItemQuantity("coin");

                if (qty >= price)
                {
                    game.eventManager.pushEvent(ADD_ITEM, std::make_any<std::pair<std::string, uint32_t>>(name, 1));
                    game.eventManager.pushEvent(REMOVE_ITEM, std::make_any<std::pair<std::string, uint32_t>>("coin", price));
                    done = true;
                    game.playSound("cash");
                    game.cutsceneManager.queueCommand(new Command_Textbox(game, "Thanks for your purchase."));
                    game.cutsceneManager.queueCommand(new Command_Callback([this]() {
                        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [this]() {
                            triggered = false;
                            });
                        }));
                }
                else
                {
                    game.cutsceneManager.queueCommand(new Command_Textbox(game, "You can't afford this item."));
                    game.cutsceneManager.queueCommand(new Command_Callback([this]() {
                        game.eventManager.pushDelayedEvent(UNNAMED, 0.2f, nullptr, [this]() {
                            triggered = false;
                            });
                        }));
                }
            }
        }
        else
        {
            if (collided)
            {
                collided = false;
                done = false;
                game.eventManager.pushEvent(HIDE_COIN_AMOUNT);
            }
        }
    }
}

void TradeItemBehavior::reset()
{
    Behavior::reset();
    triggered = false;
    collided = false;
}

void TradeItemBehavior::draw()
{
    if (auto s = self.lock())
    {
        int x = (int)s->position.x - 4;
        int y = (int)s->position.y + 16;
        const auto& coinTex = game.loader.getTextures("itemDropCoin")[0];
        std::string priceText = "x" + std::to_string(price);
        int textW = MeasureText(priceText.c_str(), 10);
        int rectW = coinTex.width + 2 + textW;
        int rectH = std::max(coinTex.height, 10);
        DrawRectangle(x, y, rectW, rectH, Color{ 0, 0, 0, 128 });
        DrawTexture(coinTex, x, y, WHITE);
        DrawText(priceText.c_str(), x + 8, y, 10, WHITE);
    }
}
