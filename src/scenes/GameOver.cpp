#include "GameOver.h"
#include "Game.h"
#include "raylib.h"
#include "InGame.h"
#include "Controls.h"


void GameOver::startup() {
    game.playSound("gameover");

    Sprite* player = game.getPlayer();
    if (player)
        player->moveTo(game.gameScreenWidth / 2.0f - ((player->lastDirection == LEFT) ? 16 : 0), game.gameScreenHeight / 2.0f);

    game.eventManager.pushDelayedEvent("advanceGameOver", 2.0f, nullptr, [this]() {
        showText1 = true;
        music = &const_cast<Music&>(game.loader.getMusic("gameover"));
        PlayMusicStream(*music);
        
        Sprite* player = game.getPlayer();
        if (player)
            player->rotationAngle = 90.0f * ((player->lastDirection == LEFT) ? 1 : -1);
    });
    game.eventManager.pushDelayedEvent("advanceGameOver2", 4.0f, nullptr, [this]() {
        showText2 = true;
     });
}

void GameOver::update(float dt) {
    if (showText2 && AnyKeyPressed(game.buttonsPressed)) {
        // restart the game
        // TODO: show a menu (restart, end)
        game.restart();
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

    if (showText2) {
        const char* text = "press any key";
        int fontSize = 14;
        int textWidth = MeasureText(text, fontSize);
        int x = (game.gameScreenWidth - textWidth) / 2;
        int y = (game.gameScreenHeight - fontSize) / 4 * 3;
        DrawText(text, x, y, fontSize, LIGHTGRAY);
    }
}

