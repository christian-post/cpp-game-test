#include "Events.h"
#include "Commands.h"
#include "ChaseBehavior.h"
#include "DialogueBehavior.h"
#include "TileMap.h"
#include "raylib.h"
#include "raymath.h"


// any InGame events (like cutscenes) that are triggered by some condition
// once triggered, they never trigger again
// TODO these get replaced with Lua scripts

void setupConditionalEvents(InGame& inGame)
{
    auto& game = inGame.getGame();

    game.eventManager.pushConditionalEvent(
        [&]() {
            // this event advances the room state of the starting room after the player got the sword
            // TODO: stateful Tiled Map objects (like chests, enemies etc) should probably have the ability to advance the Room state directly, like doors already do
            if (!inGame.tileMap) 
                return false;
            return (game.inventory.getItemQuantity("weapon_sword") > 0);
        },
        [&]() {
            size_t level = 0;
            size_t roomIdx = game.currentWorld->startingRoomIndex;
            game.currentWorld->advanceRoomState(level, roomIdx);
        }
    );

    /*
    game.eventManager.pushConditionalEvent(
        [&]() {
            // TODO just a test
            const TileMap* tm = game.currentWorld->getCurrentTileMap();
            if (!tm)
                return false;
            const std::string& roomID = tm->getRoomID();
            return (roomID == "test_room" && game.currentWorld->getCurrentRoom()->state < 2);
        },
        [&]() {

            game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
                
                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.eventManager.pushEvent(HIDE_HUD);
                    }));

                game.cutsceneManager.queueCommand(new Command_Letterbox(game.gameScreenWidth, game.gameScreenHeight, 2.0f, false));
                game.cutsceneManager.queueCommand(new Command_Wait(1.0f));

                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    TraceLog(LOG_INFO, "SOME EVENT");
                    }));

                game.cutsceneManager.queueCommand(new Command_Wait(1.0f));

                game.cutsceneManager.queueCommand(new Command_Letterbox(game.gameScreenWidth, game.gameScreenHeight, 2.0f, true));

                game.cutsceneManager.queueCommand(new Command_Callback([&]() {
                    game.eventManager.pushEvent(SHOW_HUD);
                    game.currentWorld->advanceRoomState();
                    }));
            });
        }
    );
    */

    //game.eventManager.pushConditionalEvent(
    //    [&]() {
    //        // this event sets the position of the elf companion next to the player after loading the first room
    //        const TileMap* tm = game.currentWorld->getCurrentTileMap();

    //        if (!tm)
    //            return false;

    //        const std::string& roomID = tm->getRoomID();
    //        return (roomID == "starting_room" && game.currentWorld->getCurrentRoom()->state >= 6);
    //    },
    //    [&]() {
    //        if (game.spriteMap.find("elfCompanion2") == game.spriteMap.end())
    //            return;

    //        game.eventManager.pushDelayedEvent(UNNAMED, 0.0f, nullptr, [&]() {
    //            Sprite& npcRef = *game.spriteMap["elfCompanion2"];
    //            Vector2 playerPos = game.getPlayer()->position;
    //            npcRef.moveTo(playerPos.x - 16.0f, playerPos.y);
    //            if (!npcRef.persistent)
    //            {
    //                npcRef.persistent = true;
    //                npcRef.followsPlayer = true;
    //                npcRef.speed = game.getSetting<float>("npcFollowSpeed");
    //                npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, game.spriteMap["elfCompanion2"], game.spriteMap["player"], 20.0f));
    //            }
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player has defeated all the enemies in the room left of the entrance
    //    [&]() {
    //        if (!inGame.tileMap) 
    //            return false;

    //        return inGame.tileMap->getName() == "dungeon004" &&
    //            game.currentWorld->getCurrentRoom()->state < 2 &&
    //            std::none_of(game.sprites.begin(), game.sprites.end(),
    //                [](const std::shared_ptr<Sprite>& s) {
    //                    return s->isEnemy;
    //                });
    //    },
    //    [&]() {
    //        TraceLog(LOG_INFO, "enemies defeated");
    //        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
    //            game.eventManager.pushEvent(HIDE_HUD);
    //            game.cutsceneManager.queueCommand(new Command_CameraPan(game, 110.0f, 20.0f, 1.0f));
    //            game.cutsceneManager.queueCommand(new Command_Wait(0.3f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                int eventKey = EventKeyRegistry::getEventKey("door004open");
    //                game.eventManager.pushEvent(eventKey);
    //                game.playSound("doorOpen_2");
    //                }));
    //            game.cutsceneManager.queueCommand(new Command_Wait(1.5f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent(SHOW_HUD);
    //                game.cutsceneManager.setCameraControl(false);
    //                game.currentWorld->advanceRoomState();
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player has defeated the demon in room 006
    //    [&]() {
    //        if (!inGame.tileMap) 
    //            return false;

    //        return inGame.tileMap->getName() == "dungeon006" &&
    //            game.currentWorld->getCurrentRoom()->state < 2 &&
    //            std::none_of(game.sprites.begin(), game.sprites.end(),
    //                [](const std::shared_ptr<Sprite>& s) {
    //                    return s->isEnemy;
    //                });
    //    },
    //    [&]() {
    //        TraceLog(LOG_INFO, "enemies defeated");
    //        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
    //            game.eventManager.pushEvent(HIDE_HUD);
    //            game.cutsceneManager.queueCommand(new Command_CameraPan(game, 110.0f, 20.0f, 1.0f));
    //            game.cutsceneManager.queueCommand(new Command_Wait(0.3f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                int eventKey = EventKeyRegistry::getEventKey("door006open");
    //                game.eventManager.pushEvent(eventKey);
    //                game.playSound("doorOpen_2");
    //                }));
    //            game.cutsceneManager.queueCommand(new Command_Wait(1.5f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent(SHOW_HUD);
    //                game.cutsceneManager.setCameraControl(false);
    //                game.currentWorld->advanceRoomState();
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    [&]() {
    //        // give your companion a different dialogue after the last room
    //        // TODO change because the shop isn't the last room anymore
    //        if (!inGame.tileMap) 
    //            return false;

    //        return (inGame.tileMap->getName() == "dungeon_shop");
    //    },
    //    [&]() {
    //        if (game.spriteMap.find("elfCompanion2") == game.spriteMap.end())
    //            return;
    //        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
    //            Sprite& npcRef = *game.spriteMap["elfCompanion2"];
    //            npcRef.removeAllBehaviors();
    //            npcRef.addBehavior(std::make_unique<ChaseBehavior>(game, game.spriteMap["elfCompanion2"], game.spriteMap["player"], 20.0f));
    //            std::string textKey = "elfDialogue3";
    //            std::vector<std::string> texts = game.loader.getText(textKey);
    //            npcRef.addBehavior(std::make_unique<DialogueBehavior>(game, game.spriteMap["elfCompanion2"], game.spriteMap["player"], texts, "powerUp4"));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player has defeated all the turrets across the moat (needs bow)
    //    [&]() {
    //        if (!inGame.tileMap)
    //            return false;

    //        return inGame.tileMap->getName() == "dungeon_turrets_0101" &&
    //            game.currentWorld->getCurrentRoom()->state < 2 &&
    //            std::none_of(game.sprites.begin(), game.sprites.end(),
    //                [](const std::shared_ptr<Sprite>& s) {
    //                    return s->isEnemy;
    //                });
    //    },
    //    [&]() {
    //        TraceLog(LOG_INFO, "enemies defeated");
    //        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
    //            game.eventManager.pushEvent(HIDE_HUD);
    //            game.cutsceneManager.queueCommand(new Command_CameraPan(game, 110.0f, 242.0f, 1.0f));
    //            game.cutsceneManager.queueCommand(new Command_Wait(0.3f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                int eventKey = EventKeyRegistry::getEventKey("doorTurretsDefeatOpen");
    //                game.eventManager.pushEvent(eventKey);
    //                game.playSound("doorOpen_2");
    //                }));
    //            game.cutsceneManager.queueCommand(new Command_Wait(1.5f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent(SHOW_HUD);
    //                game.cutsceneManager.setCameraControl(false);
    //                game.currentWorld->advanceRoomState();
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player has defeated all the enemies in dungeon_fight_chest_0100
    //    [&]() {
    //        if (!inGame.tileMap)
    //            return false;

    //        return inGame.tileMap->getName() == "dungeon_fight_chest_0100" &&
    //            game.currentWorld->getCurrentRoom()->state < 2 &&
    //            std::none_of(game.sprites.begin(), game.sprites.end(),
    //                [](const std::shared_ptr<Sprite>& s) {
    //                    return s->isEnemy;
    //                });
    //    },
    //    [&]() {
    //        TraceLog(LOG_INFO, "enemies defeated");
    //        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
    //            game.eventManager.pushEvent(HIDE_HUD);
    //            game.cutsceneManager.queueCommand(new Command_CameraPan(game, 136.0f, 118.0f, 1.0f));
    //            game.cutsceneManager.queueCommand(new Command_Wait(0.3f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.currentWorld->advanceRoomState();
    //                game.eventManager.pushEvent(RELOAD_ROOM); // make the chest appear
    //                }));
    //            game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent(SHOW_HUD);
    //                game.cutsceneManager.setCameraControl(false);
    //                }));
    //            });
    //    }
    //);

    //game.eventManager.pushConditionalEvent(
    //    // the player has defeated the final boss
    //    [&]() {
    //        if (!inGame.tileMap)
    //            return false;

    //        return inGame.tileMap->getName() == "dungeon_final_boss_0001" &&
    //            game.currentWorld->getCurrentRoom()->state < 4 &&
    //            std::none_of(game.sprites.begin(), game.sprites.end(),
    //                [](const std::shared_ptr<Sprite>& s) {
    //                    return s->isEnemy;
    //                });
    //    },
    //    [&]() {
    //        TraceLog(LOG_INFO, "enemies defeated");
    //        game.eventManager.pushDelayedEvent(UNNAMED, 0.1f, nullptr, [&]() {
    //            game.eventManager.pushEvent(HIDE_HUD);
    //            game.cutsceneManager.queueCommand(new Command_CameraPan(game, 110.0f, 242.0f, 1.0f));
    //            game.cutsceneManager.queueCommand(new Command_Wait(0.3f));
    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                int eventKey = EventKeyRegistry::getEventKey("doorFinalBossOpen");
    //                game.eventManager.pushEvent(eventKey);
    //                game.playSound("doorOpen_2");
    //                }));
    //            game.cutsceneManager.queueCommand(new Command_Wait(1.5f));

    //            if (!(game.spriteMap.find("elfCompanion2") == game.spriteMap.end()))
    //            {
    //                Sprite& npcRef = *game.spriteMap["elfCompanion2"];
    //                Vector2 playerPos = game.getPlayer()->position;
    //                game.cutsceneManager.queueCommand(new Command_CameraPan(game, playerPos.x, playerPos.y, 0.5f));
    //                game.cutsceneManager.queueCommand(new Command_MoveTo(npcRef, playerPos.x - 16.0f, playerPos.y, 0.5f));
    //                game.cutsceneManager.queueCommand(new Command_LookTowards(npcRef, *game.getPlayer()));
    //                game.cutsceneManager.queueCommand(new Command_LookTowards(*game.getPlayer(), npcRef));
    //                game.cutsceneManager.queueCommand(new Command_Wait(0.5f));
    //                std::string textKey = "elfDialogue4";
    //                std::vector<std::string> texts = game.loader.getText(textKey);
    //                game.cutsceneManager.queueCommand(new Command_Textbox(game, texts[0], "powerUp4", true)); // TODO pass the key to texts.json directly instead of the actual dialogue string... 
    //            }

    //            game.cutsceneManager.queueCommand(new Command_Callback([&]() {
    //                game.eventManager.pushEvent(SHOW_HUD);
    //                game.cutsceneManager.setCameraControl(false);
    //                // TODO state 2 closes the door
    //                game.currentWorld->advanceRoomState();
    //                game.currentWorld->advanceRoomState();
    //                }));
    //            });
    //    }
    //);
}
