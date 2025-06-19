#include "Events.h"
#include "raylib.h"
#include "raymath.h"
#include "Commands.h"
#include "Behavior.h"

void setupConditionalEvents(InGame& inGame) {
    auto& game = inGame.getGame();

    game.eventManager.pushConditionalEvent(
        // the player has defeated all the enemies in the second room
        // TODO: change the conditions?
        [&]() {
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
                    }));
                });
            game.currentDungeon->advanceRoomState();
            //inGame.advanceRoomState("test_dungeon");
        }
    );

    //game.eventManager.pushConditionalEvent(
    //    // Start of the game
    //    [&]() {
    //        return inGame.tileMap->getName() == "test_dungeon2" && inGame.roomStates["test_dungeon2"] == 1;
    //    },
    //    [&]() {
    //        game.eventManager.pushDelayedEvent("cutsceneStart", 0.1f, nullptr, [&]() {
    //            game.eventManager.pushEvent("hideHUD");
    //            game.cutsceneManager.queueCommand(new Command_Letterbox(float(game.gameScreenWidth), float(game.gameScreenHeight), 1.0f), false);
    //            Sprite& npcRef = *inGame.spriteMap["elfCompanion"];
    //            float npcX = npcRef.position.x;
    //            float npcY = npcRef.position.y + 3.0f * (float)inGame.tileMap->tileHeight;
    //            game.cutsceneManager.queueCommand(new Command_Wait(1.0f));
    //            game.cutsceneManager.queueCommand(new Command_MoveTo(npcRef, npcX, npcY, 2.0f));
    //            game.cutsceneManager.queueCommand(new Command_Look(*inGame.player, LEFT));
    //            game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
    //            game.cutsceneManager.queueCommand(new Command_Textbox(game, "Please help me, Sir Knight! These skeletons over there are up to no good. Do something about them!!!"));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent("showHUD");
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player has defeated all the enemies in the first room
    //    [&]() {
    //        return inGame.tileMap->getName() == "test_dungeon2" &&
    //            std::none_of(game.sprites.begin(), game.sprites.end(),
    //                [](const std::shared_ptr<Sprite>& s) {
    //                    return s->isEnemy;
    //                });
    //    },
    //    [&]() {
    //        TraceLog(LOG_INFO, "enemies defeated");
    //        game.eventManager.pushDelayedEvent("defeatDialog", 0.1f, nullptr, [&]() {
    //            game.eventManager.pushEvent("hideHUD");
    //            game.eventManager.pushEvent("door1open");
    //            game.playSound("doorOpen_2");

    //            game.cutsceneManager.queueCommand(new Command_Letterbox(float(game.gameScreenWidth), float(game.gameScreenHeight), 1.0f), false);
    //            Sprite& npcRef = *inGame.spriteMap["elfCompanion"];
    //            game.cutsceneManager.queueCommand(new Command_MoveTo(npcRef, 9.0f * float(inGame.tileSize), 6.0f * float(inGame.tileSize), 2.0f), false);
    //            game.cutsceneManager.queueCommand(new Command_Look(npcRef, RIGHT));
    //            game.cutsceneManager.queueCommand(new Command_MoveTo(*inGame.player, 10.0f * float(inGame.tileSize), 6.0f * float(inGame.tileSize), 2.0f));
    //            game.cutsceneManager.queueCommand(new Command_Look(*inGame.player, LEFT));
    //            game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
    //            game.cutsceneManager.queueCommand(new Command_Textbox(game, "You defeated all of the skeletons. Now the door opened for some reason. I will follow you into the next room."));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent("showHUD");
    //                npcRef.removeAllBehaviors();
    //                npcRef.speed = 16;
    //                npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, inGame.spriteMap["elfCompanion"], inGame.player, 1000.0f, 12.0f, 2000.0f));
    //                npcRef.persistent = true;
    //                inGame.advanceRoomState("test_dungeon2");
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player enters the second room for the first time
    //    [&]() {
    //        return inGame.tileMap->getName() == "test_dungeon" && inGame.roomStates["test_dungeon"] == 1;
    //    },
    //    [&]() {
    //        game.eventManager.pushDelayedEvent("room2Start", 0.01f, nullptr, [&]() {
    //            Sprite& npcRef = *inGame.spriteMap["elfCompanion"];
    //            npcRef.removeAllBehaviors();
    //            game.cutsceneManager.queueCommand(new Command_MoveTo(npcRef, 17.0f * float(inGame.tileSize), 21.0f * float(inGame.tileSize), 0.0f), false);
    //            game.eventManager.pushEvent("hideHUD");
    //            game.cutsceneManager.queueCommand(new Command_Letterbox(float(game.gameScreenWidth), float(game.gameScreenHeight), 1.0f), false);
    //            game.cutsceneManager.queueCommand(new Command_MoveTo(*inGame.player, 18.0f * float(inGame.tileSize), 22.0f * float(inGame.tileSize) - 4.0f, 0.1f));
    //            game.cutsceneManager.queueCommand(new Command_Textbox(game, "This room is plagued with skeletons and other baddies as well.\nYou have to find and defeat them all to open the door that gets us out of here.\nBut I can't help you since I don't have any abilities yet. Good luck!"));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent("showHUD");
    //                inGame.advanceRoomState("test_dungeon");
    //                npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, inGame.spriteMap["elfCompanion"], inGame.player, 1000.0f, 12.0f, 2000.0f));
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player has defeated all the enemies in the second room
    //    [&]() {
    //        return inGame.tileMap->getName() == "test_dungeon" &&
    //            std::none_of(game.sprites.begin(), game.sprites.end(),
    //                [](const std::shared_ptr<Sprite>& s) {
    //                    return s->isEnemy;
    //                });
    //    },
    //    [&]() {
    //        TraceLog(LOG_INFO, "enemies defeated");
    //        game.eventManager.pushDelayedEvent("defeatDialog", 0.1f, nullptr, [&]() {
    //            game.eventManager.pushEvent("hideHUD");
    //            game.eventManager.pushEvent("door2open");
    //            game.playSound("doorOpen_2");
    //            inGame.advanceRoomState("test_dungeon");

    //            game.cutsceneManager.queueCommand(new Command_Letterbox(float(game.gameScreenWidth), float(game.gameScreenHeight), 1.0f), false);
    //            Sprite& npcRef = *inGame.spriteMap["elfCompanion"];
    //            game.cutsceneManager.queueCommand(new Command_Look(npcRef, RIGHT));
    //            game.cutsceneManager.queueCommand(new Command_Look(*inGame.player, LEFT));
    //            game.cutsceneManager.queueCommand(new Command_Wait(1.5f));
    //            game.cutsceneManager.queueCommand(new Command_Textbox(game, "Cool stuff. Let's get outta here!"));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent("showHUD");
    //                npcRef.removeAllBehaviors();
    //                npcRef.speed = 16;
    //                npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, inGame.spriteMap["elfCompanion"], inGame.player, 1000.0f, 12.0f, 2000.0f));
    //                npcRef.persistent = true;
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player enters the third room for the first time
    //    [&]() {
    //        return inGame.tileMap->getName() == "test_dungeon1" && inGame.roomStates["test_dungeon1"] == 1;
    //    },
    //    [&]() {
    //        game.eventManager.pushDelayedEvent("room3Start", 0.01f, nullptr, [&]() {
    //            Sprite& npcRef = *inGame.spriteMap["elfCompanion"];
    //            npcRef.removeAllBehaviors();
    //            game.cutsceneManager.queueCommand(new Command_MoveTo(npcRef, 12.0f * float(inGame.tileSize), 20.0f * float(inGame.tileSize), 0.0f), false);
    //            game.eventManager.pushEvent("hideHUD");
    //            game.cutsceneManager.queueCommand(new Command_Letterbox(float(game.gameScreenWidth), float(game.gameScreenHeight), 1.0f), false);
    //            game.cutsceneManager.queueCommand(new Command_MoveTo(*inGame.player, 11.0f * float(inGame.tileSize), 20.0f * float(inGame.tileSize), 1.0f));
    //            game.cutsceneManager.queueCommand(new Command_Textbox(game, "This room is strange. Something feels off."));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent("showHUD");
    //                inGame.advanceRoomState("test_dungeon1");
    //                npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, inGame.spriteMap["elfCompanion"], inGame.player, 1000.0f, 12.0f, 2000.0f));
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player has defeated all the enemies in the third room
    //    [&]() {
    //        return inGame.tileMap->getName() == "test_dungeon1" &&
    //            std::none_of(game.sprites.begin(), game.sprites.end(),
    //                [](const std::shared_ptr<Sprite>& s) {
    //                    return s->isEnemy;
    //                });
    //    },
    //    [&]() {
    //        TraceLog(LOG_INFO, "enemies defeated");
    //        game.eventManager.pushDelayedEvent("room3Cleared", 1.0f, nullptr, [&]() {
    //            game.eventManager.pushEvent("door3open");
    //            game.playSound("doorOpen_2");
    //            inGame.advanceRoomState("test_dungeon1");
    //            });
    //    }
    //);
}
