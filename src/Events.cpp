#include "Events.h"
#include "raylib.h"
#include "raymath.h"
#include "Commands.h"
#include "Behavior.h"

// any InGame events (like cutscenes) that are triggered by some condition
// once triggered, they never trigger again

void setupConditionalEvents(InGame& inGame) {
    auto& game = inGame.getGame();

    game.eventManager.pushConditionalEvent(
        [&]() {
            // this event advances the room state after the player got the sword from the chest
            // TODO: stateful Tiled Map objects (like chests, enemies etc) should probably have the ability to advance the Room state directly, like doors already do
            if (!inGame.tileMap) 
                return false;
            return (inGame.tileMap->getName() == "dungeon005" && game.inventory.getItemQuantity("weapon_sword") > 0);
        },
        [&]() {
            game.currentDungeon->advanceRoomState(14);
        }
    );

    game.eventManager.pushConditionalEvent(
        [&]() {
            // this happens in the first room after the player obtained the sword from the other room
            if (!inGame.tileMap) 
                return false;
            return (inGame.tileMap->getName() == "dungeon001" && game.inventory.getItemQuantity("weapon_sword") > 0);
            },
        [&]() {
            if (game.spriteMap.find("elfCompanion2") == game.spriteMap.end())
                return;
            game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
                Sprite& npcRef = *game.spriteMap["elfCompanion2"];               
                game.eventManager.pushEvent(HIDE_HUD);
                game.cutsceneManager.queueCommand(new Command_Letterbox(float(game.gameScreenWidth), float(game.gameScreenHeight), 1.0f), false);
                float npcX = 12.0f * static_cast<float>(inGame.tileSize);
                float npcY = 8.0f * static_cast<float>(inGame.tileSize);
                game.cutsceneManager.queueCommand(new Command_Wait(1.0f));
                game.cutsceneManager.queueCommand(new Command_MoveTo(npcRef, npcX, npcY, 2.0f));
                game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
                game.cutsceneManager.queueCommand(new Command_Textbox(game, "Is that a sword? Great! I'll follow you, now we can fight our way out of here.", "powerUp4", true)); // TODO pass a key to a text in texts.json instead of the actual dialogue string... 
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.eventManager.pushEvent(SHOW_HUD);
                    game.currentDungeon->advanceRoomState();
                    if (!npcRef.persistent) {
                        npcRef.persistent = true;
                        npcRef.followsPlayer = true;
                        npcRef.speed = 16;
                        npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, game.spriteMap["elfCompanion2"], game.spriteMap["player"], 1000.0f, 20.0f, 2000.0f));
                    }
                    }));
                });
        }
    );

    game.eventManager.pushConditionalEvent(
        // the player has defeated all the enemies in the room left of the entrance
        [&]() {
            if (!inGame.tileMap) 
                return false;
            return inGame.tileMap->getName() == "dungeon004" &&
                game.currentDungeon->getCurrentRoomState() < 2 &&
                std::none_of(game.sprites.begin(), game.sprites.end(),
                    [](const std::shared_ptr<Sprite>& s) {
                        return s->isEnemy;
                    });
        },
        [&]() {
            TraceLog(LOG_INFO, "enemies defeated");
            game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
                game.eventManager.pushEvent(HIDE_HUD);
                game.cutsceneManager.queueCommand(new Command_CameraPan(game, 110.0f, 20.0f, 1.0f));
                game.cutsceneManager.queueCommand(new Command_Wait(0.3f));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    int eventKey = EventKeyRegistry::getEventKey("door004open");
                    game.eventManager.pushEvent(eventKey);
                    game.playSound("doorOpen_2");
                    }));
                game.cutsceneManager.queueCommand(new Command_Wait(1.5f));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.eventManager.pushEvent(SHOW_HUD);
                    game.cutsceneManager.setCameraControl(false);
                    game.currentDungeon->advanceRoomState();
                    }));
                });      
        }
    );

    game.eventManager.pushConditionalEvent(
        // the player has defeated the demon in room 006
        [&]() {
            if (!inGame.tileMap) 
                return false;
            return inGame.tileMap->getName() == "dungeon006" &&
                game.currentDungeon->getCurrentRoomState() < 2 &&
                std::none_of(game.sprites.begin(), game.sprites.end(),
                    [](const std::shared_ptr<Sprite>& s) {
                        return s->isEnemy;
                    });
        },
        [&]() {
            TraceLog(LOG_INFO, "enemies defeated");
            game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
                game.eventManager.pushEvent(HIDE_HUD);
                game.cutsceneManager.queueCommand(new Command_CameraPan(game, 110.0f, 20.0f, 1.0f));
                game.cutsceneManager.queueCommand(new Command_Wait(0.3f));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    int eventKey = EventKeyRegistry::getEventKey("door006open");
                    game.eventManager.pushEvent(eventKey);
                    game.playSound("doorOpen_2");
                    }));
                game.cutsceneManager.queueCommand(new Command_Wait(1.5f));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.eventManager.pushEvent(SHOW_HUD);
                    game.cutsceneManager.setCameraControl(false);
                    game.currentDungeon->advanceRoomState();
                    }));
                });
        }
    );

    game.eventManager.pushConditionalEvent(
        [&]() {
            // give your companion a different dialogue after the last room
            if (!inGame.tileMap) 
                return false;
            return (inGame.tileMap->getName() == "dungeon_shop");
        },
        [&]() {
            if (game.spriteMap.find("elfCompanion2") == game.spriteMap.end())
                return;
            game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
                Sprite& npcRef = *game.spriteMap["elfCompanion2"];
                npcRef.removeAllBehaviors();
                npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, game.spriteMap["elfCompanion2"], game.spriteMap["player"], 1000.0f, 20.0f, 2000.0f));
                std::string textKey = "elfDialogue3";
                std::vector<std::string> texts = game.loader.getText(textKey);
                npcRef.addBehavior(std::make_unique<DialogueBehavior>(game, game.spriteMap["elfCompanion2"], game.spriteMap["player"], texts, "powerUp4"));
                });
        }
    );
}
