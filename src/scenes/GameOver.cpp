#include "GameOver.h"
#include "Game.h"
#include "InGame.h"
#include "Controls.h"


void GameOver::startup() {
    game.playSound("gameover");

    // configure the menu
    menu.setItems({
        {
            "Save and Try Again",
            MenuItemType::Action,
            [&]() {
                game.stopScene(getName());
                game.setOnSceneComplete("WriteSavegameMenu", [&game = game]() {
                    game.restart(); // go to title screen after saving
                });
                game.startScene("WriteSavegameMenu");
            }
        },
        {
            "Quit without saving",
            MenuItemType::Action,
            [&]() {
                game.restart();
            }
        }
        });

    menu.restrictHeight(game.gameScreenHeight / 2, game.gameScreenHeight / 2 + 16);

    // move the player to the center
    Sprite* player = game.getPlayer();
    if (player)
        player->moveTo(game.gameScreenWidth / 2.0f - ((player->lastDirection == LEFT) ? 16 : 0), game.gameScreenHeight / 2.0f);

    // delayed events that advance the state of this scene
    game.eventManager.pushDelayedEvent(UNNAMED, 2.0f, nullptr, [this]() {
        showText1 = true;
        music = &const_cast<Music&>(game.loader.getMusic("gameover"));
        PlayMusicStream(*music);
        
        Sprite* player = game.getPlayer();
        if (player)
            player->rotationAngle = 90.0f * ((player->lastDirection == LEFT) ? 1 : -1);
    });
    game.eventManager.pushDelayedEvent(UNNAMED, 4.0f, nullptr, [this]() {
        // only shows the menu items after a brief moment
        showMenu = true;
     });
}

void GameOver::update(float dt) {
    if (showMenu) {
        menu.update();
    }
}

void GameOver::draw() {
    // draw the player over a black background
    ClearBackground(BLACK);

    Sprite* player = game.getPlayer();
    if (player) {
        player->iFrameTimer = 0.0f;
        player->draw();
    }

    if (showText1) {
        const char* text = "GAME OVER";
        int fontSize = 30;
        // Center the text
        int textWidth = MeasureText(text, fontSize);
        int x = (game.gameScreenWidth - textWidth) / 2;
        int y = (game.gameScreenHeight - fontSize) / 3;
        DrawText(text, x, y, fontSize, LIGHTGRAY);
    }

    if (showMenu) {
        menu.draw();
    }
}

