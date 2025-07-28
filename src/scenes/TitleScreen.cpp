#include "TitleScreen.h"
#include "Game.h"
#include "raylib.h"
#include "Controls.h"
#include "Utils.h"


void TitleScreen::startup() {
    TraceLog(LOG_INFO, "Title Screen Startup\n");

    music = &const_cast<Music&>(game.loader.getMusic("title"));
    PlayMusicStream(*music);
}

void TitleScreen::update(float deltaTime) {
    transparency = (unsigned char)((sinf(float(GetTime()) * PI) + 1.0f) * 127.5f);
    if (AnyKeyPressed(game.buttonsPressed)) {
        game.startScene("StartMenu");
        game.stopScene(getName());
    }
}

void TitleScreen::draw() {
    ClearBackground(BLACK);

    DrawTexture(game.loader.getTextures("title_image")[0], 0, 0, WHITE);

    const char* promptText = "Press any key to play";
    int fontSize = 12;
    // Center text
    int promptWidth = MeasureText(promptText, fontSize);
    int promptX = (game.gameScreenWidth - promptWidth) / 2;
    int titleY = game.gameScreenHeight / 3;
    int promptY = game.gameScreenHeight - 24;

    Color color = { 255, 255, 255, transparency };
    Font defaultFont = GetFontDefault();
    DrawTextEx(defaultFont, promptText, { float(promptX), float(promptY) }, float(fontSize), 1.0f, color);

}

void TitleScreen::end() {
    // wait for a split second
    TraceLog(LOG_INFO, "TitleScreen has ended");
    WaitTime(0.25f);
    StopMusicStream(*music);
}
