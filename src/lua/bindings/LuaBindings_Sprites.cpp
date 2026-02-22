#include "LuaBindings.h"
#include "Game.h"
#include "InGame.h"
#include "ChaseBehavior.h"
#include "DialogueBehavior.h"

void bindSpriteOperations(sol::state& lua, Game& game, InGame& inGame)
{
    lua.set_function("setSpriteProperty", [&](std::string spriteName, std::string property, sol::object value) {
        if (game.spriteMap.find(spriteName) == game.spriteMap.end())
            return;

        Sprite& sprite = *game.spriteMap[spriteName];

        if (property == "persistent" && value.is<bool>())
            sprite.persistent = value.as<bool>();
        else if (property == "followsPlayer" && value.is<bool>())
            sprite.followsPlayer = value.as<bool>();
        else if (property == "speed" && value.is<float>())
            sprite.speed = value.as<float>();
        });

    lua.set_function("addSpriteBehavior", [&](std::string spriteName, std::string behaviorType, sol::table config) {
        if (game.spriteMap.find(spriteName) == game.spriteMap.end())
            return;

        // TODO add more behaviors

        if (behaviorType == "ChaseBehavior")
        {
            std::string targetName = config["target"].get_or(std::string("player"));
            float distance = config["distance"].get_or(20.0f);

            if (game.spriteMap.find(targetName) != game.spriteMap.end())
            {
                game.spriteMap[spriteName]->addBehavior(
                    std::make_unique<ChaseBehavior>(
                        game,
                        game.spriteMap[spriteName],
                        game.spriteMap[targetName],
                        distance
                    )
                );
            }
        }
        else if (behaviorType == "DialogueBehavior")
        {
            std::string targetName = config["target"].get_or(std::string("player"));
            std::string textKey = config["textKey"].get_or(std::string(""));
            std::string portrait = config["portrait"].get_or(std::string(""));

            if (!textKey.empty() && game.spriteMap.find(targetName) != game.spriteMap.end())
            {
                std::vector<std::string> texts = game.loader.getText(textKey);
                game.spriteMap[spriteName]->addBehavior(
                    std::make_unique<DialogueBehavior>(
                        game,
                        game.spriteMap[spriteName],
                        game.spriteMap[targetName],
                        texts,
                        portrait
                    )
                );
            }
        }
        });
}