#include "TitleScreen.h"
#include "Game.h"
#include "raylib.h"
#include "Controls.h"
#include "Utils.h"


void TitleScreen::startup() {
    Log("Title Screen Startup\n");
}

void TitleScreen::update(float dt) {
    if (AnyKeyPressed(game.buttonsPressed)) {
        game.startScene("InGame");
        game.startScene("HUD");
        game.stopScene(getName());
    }
}

void TitleScreen::draw() {
    ClearBackground(BLACK);

    const char* titleText = "This is the title screen";
    const char* promptText = "Press any key to play";
    int fontSize = 20;

    // Center text
    int titleWidth = MeasureText(titleText, fontSize);
    int promptWidth = MeasureText(promptText, fontSize);
    int titleX = (game.gameScreenWidth - titleWidth) / 2;
    int promptX = (game.gameScreenWidth - promptWidth) / 2;

    int titleY = game.gameScreenHeight / 3;
    int promptY = game.gameScreenHeight / 2;

    DrawText(titleText, titleX, titleY, fontSize, LIGHTGRAY);
    DrawText(promptText, promptX, promptY, fontSize, LIGHTGRAY);
}

void TitleScreen::end() {
    // wait for a split second
    Log("TitleScreen has ended");
    WaitTime(0.25f);
}
