#include "SettingsMenu.h"

SettingsMenu::SettingsMenu(Game& game, const std::string& name)
    : Scene(game, name), menu(MenuSelect(game))
{
    menu.setFontSize(6);
    menu.setYMargin(8);
}

void SettingsMenu::startup()
{
    // A bunch of settings
    // TODO to be continued

    MenuItem SoundOnItem;
    SoundOnItem.displayName = "Sound";
    SoundOnItem.type = MenuItemType::Cycle;
    SoundOnItem.options = { "On", "Off" };
    SoundOnItem.currentOption = game.getSetting<bool>("soundOn") == true ? 0 : 1;
    SoundOnItem.cycleCallback = [&](size_t index) {
        bool isOn = index == 0 ? true : false;
        game.soundOn = isOn;
        game.musicOn = isOn;
        game.writeSetting("soundOn", isOn);
        };
    menu.addItem(SoundOnItem);

    MenuItem volumeItem;
    volumeItem.displayName = "Master Volume";
    volumeItem.type = MenuItemType::Number;
    volumeItem.numberValue = game.getSetting<int>("masterVolume");
    volumeItem.minValue = 0;
    volumeItem.maxValue = 100;
    volumeItem.step = 5;
    volumeItem.numberCallback = [&](int value) {
        game.writeSetting("masterVolume", value);
        SetMasterVolume(static_cast<float>(value) / 100.0f);
        };
    menu.addItem(volumeItem);

    MenuItem musicVolItem;
    musicVolItem.displayName = "Music Volume";
    musicVolItem.type = MenuItemType::Number;
    musicVolItem.numberValue = game.getSetting<int>("musicVolume");
    musicVolItem.minValue = 0;
    musicVolItem.maxValue = 100;
    musicVolItem.step = 5;
    musicVolItem.numberCallback = [&](int value) {
        game.writeSetting("musicVolume", value);
        };
    menu.addItem(musicVolItem);

    MenuItem soundVolItem;
    soundVolItem.displayName = "Sound Effects Volume";
    soundVolItem.type = MenuItemType::Number;
    soundVolItem.numberValue = game.getSetting<int>("sfxVolume");
    soundVolItem.minValue = 0;
    soundVolItem.maxValue = 100;
    soundVolItem.step = 5;
    soundVolItem.numberCallback = [&](int value) {
        game.writeSetting("sfxVolume", value);
        };
    menu.addItem(soundVolItem);
    
    menu.addItem({ "Fullscreen On/Off", MenuItemType::Action, [&]() {
            game.toggleFullscreen();
        } });

    MenuItem fpsItem;
    fpsItem.displayName = "Max Refresh Rate";
    fpsItem.type = MenuItemType::Number;
    fpsItem.numberValue = game.getSetting<int>("targetFPS");
    fpsItem.minValue = 10;
    fpsItem.maxValue = 240;
    fpsItem.step = 10;
    fpsItem.numberCallback = [&](int value) {
        SetTargetFPS(value);
        game.writeSetting("targetFPS", value);
        };
    menu.addItem(fpsItem);

    MenuItem textSpeedItem;
    textSpeedItem.displayName = "Text Speed";
    textSpeedItem.type = MenuItemType::Cycle;
    textSpeedItem.options = { "Slow", "Medium", "Fast" };

    const float speeds[] = { 0.08f, 0.05f, 0.02f };
    float currentSpeed = game.getSetting<float>("textDelay");
    textSpeedItem.currentOption = (currentSpeed > speeds[1]) ? 0 : (currentSpeed < speeds[1]) ? 2 : 1;

    textSpeedItem.cycleCallback = [&, speeds](size_t index) {
        double rounded = std::round(speeds[index] * 100.0) / 100.0;
        game.writeSetting("textDelay", rounded);
        //game.writeSetting("textDelay", speeds[index]);
        };
    menu.addItem(textSpeedItem);

    // Last item: go back to the previous menu
    menu.addItem({ "Back", MenuItemType::Action, [&]() {
        // TODO go back to wherever you came from
        //game.eventManager.pushEvent(SELECT_MENU_DONE);
        game.startScene("StartMenu");
        game.stopScene(getName());
        } });
}

void SettingsMenu::update(float deltaTime)
{
    menu.update();
}

void SettingsMenu::draw()
{
    menu.draw();
}
