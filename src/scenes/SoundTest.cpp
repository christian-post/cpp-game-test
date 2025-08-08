#include "SoundTest.h"
#include "Game.h"
#include "Controls.h"
#include "Utils.h"
#include "raylib.h"

SoundTest::SoundTest(Game& game, const std::string& name) : Scene(game, name), menu(MenuSelect(game)) {
    menu.setFontSize(6);
    menu.setMargin(4);
}

void SoundTest::startup() {
    // TODO: list and play the sounds already loaded instead
    std::vector<std::string> files = listFiles("./resources/sound/sfx");
    for (const auto& file : files) {
        size_t pos = file.find("sfx") + 4;
        std::string substr = file.substr(pos);
        TraceLog(LOG_INFO, "%s", substr.c_str());
        menu.addItem({ substr, [this, file]() {
            if (!this->game.soundOn || !this->game.sfxOn) return;
            UnloadSound(currentSound);
            currentSound = LoadSound(file.c_str());
            PlaySound(currentSound);
            } 
        });
    }
    // go back to the start menu
    menu.addItem({ "Back", [&]() { 
        game.startScene("StartMenu");
        game.stopScene(getName());
        } });
}

void SoundTest::update(float deltaTime) {
    menu.update();
}

void SoundTest::draw() {
    menu.draw();
}

void SoundTest::end() {
    UnloadSound(currentSound);
}
