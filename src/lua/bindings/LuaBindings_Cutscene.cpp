#include "LuaBindings.h"
#include "Game.h"
#include "InGame.h"
#include "Commands.h"
#include "raylib.h"

void bindCutsceneCommands(sol::state& lua, Game& game, InGame& inGame)
{
    lua.set_function("hideHUD", [&]() {
        game.cutsceneManager.queueCommand(new Command_Callback([&]() {
            game.eventManager.pushEvent(HIDE_HUD);
            }));
        });

    lua.set_function("showHUD", [&]() {
        game.cutsceneManager.queueCommand(new Command_Callback([&]() {
            game.eventManager.pushEvent(SHOW_HUD);
            }));
        });

    lua.set_function("wait", [&](float duration) {
        game.cutsceneManager.queueCommand(new Command_Wait(duration));
        });

    lua.set_function("letterbox", [&](float width, float height, float duration, sol::optional<bool> reverse) {
        game.cutsceneManager.queueCommand(new Command_Letterbox(width, height, duration, reverse.value_or(false)));
        });

    lua.set_function("moveSpriteTo", [&](const std::string& name, float x, float y, float duration) {
        if (game.spriteMap.find(name) != game.spriteMap.end())
        {
            Sprite& sprite = *game.spriteMap[name];
            game.cutsceneManager.queueCommand(new Command_MoveTo(sprite, x, y, duration));
        }
        });

    lua.set_function("showTextbox", [&](std::string textKey, std::string voice) {
        std::vector<std::string> texts = game.loader.getText(textKey);
        if (!texts.empty())
        {
            game.cutsceneManager.queueCommand(new Command_Textbox(game, texts[0], voice, true));
        }
        });

    lua.set_function("cameraPan", [&](float x, float y, float duration) {
        game.cutsceneManager.queueCommand(new Command_CameraPan(game, x, y, duration));
        });

    lua.set_function("releaseCameraControl", [&]() {
        game.cutsceneManager.queueCommand(new Command_Callback([&]() {
            game.cutsceneManager.setCameraControl(false);
            }));
        });

    lua.set_function("playSound", [&](const std::string& soundName) {
        game.cutsceneManager.queueCommand(new Command_Callback([&, soundName]() {
            game.playSound(soundName);
            }));
        });

    lua.set_function("triggerEvent", [&](const std::string& eventKey) {
        int key = EventKeyRegistry::getEventKey(eventKey);
        game.cutsceneManager.queueCommand(new Command_Callback([&, key]() {
            game.eventManager.pushEvent(key);
            }));
        });

    lua.set_function("reloadRoom", [&]() {
        game.cutsceneManager.queueCommand(new Command_Callback([&]() {
            game.eventManager.pushEvent(RELOAD_ROOM);
            }));
        });

    lua.set_function("advanceRoomState", [&]() {
        game.cutsceneManager.queueCommand(new Command_Callback([&]() {
            game.currentWorld->advanceRoomState();
            }));
        });

    lua.set_function("onCutsceneComplete", [&](sol::function callback) {
        game.cutsceneManager.queueCommand(new Command_Callback([callback]() mutable {
            auto result = callback();
            if (!result.valid())
            {
                sol::error err = result;
                TraceLog(LOG_ERROR, "Lua callback error: %s", err.what());
            }
            }));
        });
}