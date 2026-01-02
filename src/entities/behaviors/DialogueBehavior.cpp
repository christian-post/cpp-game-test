#include "DialogueBehavior.h"
#include "Sprite.h"
#include "Game.h"
#include "InGame.h"
#include "Commands.h"
#include "Controls.h"
#include <any>
#include <tuple>

DialogueBehavior::DialogueBehavior(Game& game, std::shared_ptr<Sprite> self, std::shared_ptr<Sprite> player, std::vector<std::string> dialogTexts, std::string voice)
    : game{ game }, self{ self }, player{ player }, dialogTexts{ std::move(dialogTexts) }, voice{ voice } 
{}

void DialogueBehavior::update(float deltaTime) 
{
    if (triggered)
        return;
    if (auto s = self.lock(), p = player.lock(); s && p)
    {
        if (CheckCollisionRecs(s->rect, p->rect))
        {
            if (!collided)
            {
                game.eventManager.pushEvent(SHOW_HELP_TEXT, std::make_any<std::tuple<std::string, char, int>>(std::tuple<std::string, char, int>{"TALK", 'O', 9}));
                collided = true;
            }
            if (game.buttonsDown & CONTROL_ACTION1 && !Command_Textbox::isTextboxCooldown())
            {
                triggered = true;
                bool pitch = (voice == "tone") ? false : true;
                game.cutsceneManager.queueCommand(new Command_LookTowards(*s, *p));
                game.cutsceneManager.queueCommand(new Command_Textbox(game, dialogTexts[currentTextIndex], voice, pitch));
                game.cutsceneManager.queueCommand(new Command_Callback([this]() {
                    game.eventManager.pushDelayedEvent(UNNAMED, 0.3f, nullptr, [this]() {
                        if (currentTextIndex < dialogTexts.size() - 1)
                            ++currentTextIndex;
                        triggered = false;
                        });
                    }));
            }
        }
        else
        {
            if (collided)
            {
                collided = false;
                game.eventManager.pushEvent(HIDE_HELP_TEXT);
            }
        }
    }
}

void DialogueBehavior::reset() 
{
    Behavior::reset();
    triggered = false;
    collided = false;
    currentTextIndex = 0;
}
