#include "Events.h"
#include "raylib.h"
#include "raymath.h"
#include "Commands.h"
#include "Behavior.h"

void setupConditionalEvents(InGame& inGame) {
    auto& game = inGame.getGame();

    game.eventManager.pushConditionalEvent(
        [&]() {
            if (!inGame.tileMap) return false;
            return (inGame.tileMap->getName() == "dungeon005" && game.inventory.getItemQuantity("weapon_sword") > 0);
        },
        [&]() {
            game.currentDungeon->advanceRoomState(14);
        }
    );

    game.eventManager.pushConditionalEvent(
        [&]() {
            if (!inGame.tileMap) return false;
            return (inGame.tileMap->getName() == "dungeon001" && game.inventory.getItemQuantity("weapon_sword") > 0);
            },
        [&]() {
            if (inGame.spriteMap.find("elfCompanion2") == inGame.spriteMap.end())
                return;
            game.eventManager.pushDelayedEvent("dungeon001HasSword", 0.1f, nullptr, [&]() {
                Sprite& npcRef = *inGame.spriteMap["elfCompanion2"];               
                game.eventManager.pushEvent("hideHUD");
                game.cutsceneManager.queueCommand(new Command_Letterbox(float(game.gameScreenWidth), float(game.gameScreenHeight), 1.0f), false);
                float npcX = 12.0f * static_cast<float>(inGame.tileSize);
                float npcY = 8.0f * static_cast<float>(inGame.tileSize);
                game.cutsceneManager.queueCommand(new Command_Wait(1.0f));
                game.cutsceneManager.queueCommand(new Command_MoveTo(npcRef, npcX, npcY, 2.0f));
                game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
                game.cutsceneManager.queueCommand(new Command_Textbox(game, "Is that a sword? Great! I'll follow you, now we can fight our way out of here.", "powerUp4", true));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.eventManager.pushEvent("showHUD");
                    game.currentDungeon->advanceRoomState();
                    if (!npcRef.persistent) {
                        npcRef.persistent = true;
                        npcRef.followsPlayer = true;
                        npcRef.speed = 16;
                        npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, inGame.spriteMap["elfCompanion2"], inGame.player, 1000.0f, 12.0f, 2000.0f));
                    }
                    }));
                });
        }
    );

    game.eventManager.pushConditionalEvent(
        // the player has defeated all the enemies in the second room
        [&]() {
            if (!inGame.tileMap) return false;
            return inGame.tileMap->getName() == "dungeon004" &&
                std::none_of(game.sprites.begin(), game.sprites.end(),
                    [](const std::shared_ptr<Sprite>& s) {
                        return s->isEnemy;
                    });
        },
        [&]() {
            TraceLog(LOG_INFO, "enemies defeated");
            game.eventManager.pushDelayedEvent("defeatDialog", 0.1f, nullptr, [&]() {
                game.eventManager.pushEvent("hideHUD");
                game.cutsceneManager.queueCommand(new Command_CameraPan(game, 110.0f, 20.0f, 2.0f));
                game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.eventManager.pushEvent("door004open");
                    game.playSound("doorOpen_2");
                    }));
                game.cutsceneManager.queueCommand(new Command_Wait(1.5f));
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.eventManager.pushEvent("showHUD");
                    game.cutsceneManager.setCameraControl(false);
                    game.currentDungeon->advanceRoomState();
                    }));
                });      
        }
    );
}
