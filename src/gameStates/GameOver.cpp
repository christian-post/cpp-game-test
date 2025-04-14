#include "GameOver.h"
#include "Game.h"
#include "raylib.h"
#include "InGame.h"


void GameOver::startup(Game* game) {
    std::cout << "Game Over\n";
    //WaitTime(1);
    game->eventManager.pushDelayedEvent("gameOver", 2.0f, nullptr, [game, this]() {
        showText1 = true;
    });
    game->eventManager.pushDelayedEvent("gameOver2", 4.0f, nullptr, [game, this]() {
        showText2 = true;
     });
}

void GameOver::events(Game* game) {
    
}

void GameOver::update(Game* game, float dt) {
    if (showText2 && AnyKeyPressed(game->buttonsPressed)) {
        // end the game
        game->end();
    }
}

void GameOver::draw(Game* game) {
    // draw the player over a black background
    ClearBackground(BLACK);

    Sprite* player = game->getPlayer();
    if (player) {
        player->iFrameTimer = 0.0f;
        player->moveTo(game->gameScreenWidth / 2.0f, game->gameScreenHeight / 2.0f);
        player->draw();
    }
    
    if (showText1) {
        const char* text = "GAME OVER";
        int fontSize = 30;

        // Center the text
        int textWidth = MeasureText(text, fontSize);
        int x = (game->gameScreenWidth - textWidth) / 2;
        int y = (game->gameScreenHeight - fontSize) / 3;

        DrawText(text, x, y, fontSize, LIGHTGRAY);
    }

    if (showText2) {
        const char* text = "press any key";
        int fontSize = 14;

        // Center the text
        int textWidth = MeasureText(text, fontSize);
        int x = (game->gameScreenWidth - textWidth) / 2;
        int y = (game->gameScreenHeight - fontSize) / 4 * 3;

        DrawText(text, x, y, fontSize, LIGHTGRAY);
    }
}

