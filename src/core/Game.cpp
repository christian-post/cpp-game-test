#include "Game.h"
#include "Overworld.h"
#include "Dungeon.h"
#include "Controls.h"
#include "Preload.h"
#include "TitleScreen.h"
#include "StartMenu.h"
#include "SettingsMenu.h"
#include "SoundTest.h"
#include "DebugMenu.h"
#include "SelectMenu.h"
#include "LoadSavegameMenu.h"
#include "WriteSavegameMenu.h"
#include "KeyBindingMenu.h"
#include "InGame.h"
#include "HUD.h"
#include "InventoryUI.h"
#include "MapUI.h"
#include "GameOver.h"
#include "WorldTransition.h"
#include "Utils.h"
#include "Emitter.h"
#include <sstream>
#include <fstream>
#include <cassert>

Game::Game() : buttonsDown{}, buttonsPressed{}, inventory(*this) 
{
    loader.loadSettings("./resources/settings.json");
    settings = &loader.getSettings();

    // set window flags (might depend on config)
    int flags = FLAG_WINDOW_RESIZABLE;

    if (getSetting<bool>("vsync"))
        flags |= FLAG_VSYNC_HINT;
    
    SetConfigFlags(flags);

    // initialize the main window from the values found in settings.json
    // fix the aspect ratio to 4:3 if necessary
    // TODO do this after every resize?
    int winW = getSetting<int>("windowWidth");
    int winH = getSetting<int>("windowHeight");

    if (float(winH) / float(winW) != 0.75f)
        winH = int(winW * 0.75f);

    InitWindow(winW, winH, "My first game");

    // clamp window size if needed
    int maxW = GetMonitorWidth(0);
    int maxH = GetMonitorHeight(0);
    if (winW > maxW || winH > maxH)
    {
        winW = std::min(winW, maxW);
        winH = std::min(winH, maxH);
        SetWindowSize(winW, winH);
    }
    SetWindowMaxSize(maxW, maxH);
    SetWindowMinSize(320, 240);
    isFullscreen = getSetting("fullscreen", false);
    if (isFullscreen)
    {
        ToggleFullscreen();
    }
    else
    {
        int x = getSetting("windowX", static_cast<int>(GetWindowPosition().x));
        int y = getSetting("windowY", static_cast<int>(GetWindowPosition().y));
        SetWindowPosition(x, y);
    }

    // window event callbacks
    windowEvents.onResize = [&](int width, int height) {
        writeSetting("windowWidth", width);
        writeSetting("windowHeight", height);
        };
    windowEvents.onReposition = [&](Vector2 pos) {
        writeSetting("windowX", pos.x);
        writeSetting("windowY", pos.y);
        };

    InitAudioDevice();
    soundOn = getSetting<bool>("soundOn");
    SetMasterVolume(getSetting<float>("masterVolume") / 100.0f);

    // Render texture initialization, used to hold the rendering result so we can easily resize it
    // see https://github.com/raysan5/raylib/blob/master/examples/core/core_window_letterbox.c
    gameScreenWidth = getSetting("gameScreenWidth");
    gameScreenHeight = getSetting("gameScreenHeight");
    target = LoadRenderTexture(gameScreenWidth, gameScreenHeight);

    SetTargetFPS(getSetting<int>("targetFPS"));

    // initialize key bindings
    if (settings->contains("keyBindings"))
        InitializeKeyBindings((*settings)["keyBindings"]);
    else
        SetDefaultKeyBindings();

    // initialize gamepad bindings
    if (settings->contains("gamepadBindings"))
        InitializeGamepadBindings((*settings)["gamepadBindings"]);
    else
        SetDefaultGamepadBindings();

    // define all Scenes as factory functions
    // the second argument is priority for the drawing order
    // TODO does the name have to be a string?
    registerScene<Preload>("Preload", 0);
    registerScene<TitleScreen>("TitleScreen", 0);
    registerScene<StartMenu>("StartMenu", 0);
    registerScene<LoadSavegameMenu>("LoadSavegameMenu", 0);
    registerScene<WriteSavegameMenu>("WriteSavegameMenu", 1);
    registerScene<SettingsMenu>("SettingsMenu", 0);
    registerScene<KeyBindingMenu>("KeyBindingMenu", 2);
    registerScene<SoundTest>("SoundTest", 0);
    registerScene<DebugMenu>("DebugMenu", 2);
    registerScene<SelectMenu>("SelectMenu", 0);
    registerScene<InGame>("InGame", 0);
    registerScene<HUD>("HUD", 1);
    registerScene<InventoryUI>("InventoryUI", 2);
    registerScene<MapUI>("MapUI", 2);
    registerScene<GameOver>("GameOver", 2);
    registerScene<WorldTransition>("WorldTransition", 1);

    // seed the rng
    srand(static_cast<uint32_t>(time(nullptr)));
}

void Game::restart() 
{
    running = false;
    restartRequested = true;
    // make sure settings are saved before they are being loaded again
    saveSettings();
}

void Game::cleanup() 
{
    saveSettings();

    savegame = nullptr;

    UnloadRenderTexture(target);
    if (lastScreenshot)
    {
        UnloadImage(*lastScreenshot);
        lastScreenshot = nullptr;
    }
    CloseAudioDevice();
    CloseWindow();
}

nlohmann::json Game::getSetting(const std::string& key, nlohmann::json defaultValue) const 
{
    if (settings->contains(key))
        return settings->at(key);
    TraceLog(LOG_ERROR, "[Settings] Missing key: %s. Using default value.", key.c_str());
    return defaultValue;
}

void Game::writeSetting(const std::string& key, nlohmann::json value)
{
    loader.writeSetting(key, value);
    //TraceLog(LOG_INFO, "%s: %s", key.c_str(), value.dump().c_str());
}

void Game::saveSettings()
{
    if (settings)
    {
        std::ofstream file("./resources/settings.json");
        file << settings->dump(2);
        TraceLog(LOG_INFO, "Settings have been saved.");
    }
    else
    {
        TraceLog(LOG_ERROR, "Settings object not initialized or null.");
    }
}

void Game::startScene(const std::string& name) 
{
    if (sceneRegistry.count(name))
    {
        scenes[name] = sceneRegistry[name](name);

        if (scenePriorities.count(name))
            scenes[name]->setDrawPriority(scenePriorities[name]);
        else
            scenes[name]->setDrawPriority(0); // default

        // transfer the onComplete callback to this scene
        if (sceneCallbacks.count(name))
        {
            scenes[name]->setOnComplete(sceneCallbacks[name]);
            sceneCallbacks.erase(name);
        }

        scenes[name]->markForStarting();
    }
    else
    {
        TraceLog(LOG_ERROR, "Scene not registered: %s", name.c_str());
        throw std::invalid_argument("No scene with this name. See console for details.");
    }
}


void Game::stopScene(const std::string& name) 
{
    // Marks the scene for removal
    if (scenes.count(name)) 
    {
        scenes[name]->markForDeletion(); 
        scenes[name]->complete();
    }
}

void Game::setSceneState(const std::string& name, bool active, bool paused) 
{
    if (scenes.count(name)) 
    {  
        if (paused)
            scenes[name]->onPause(); // TODO is it weird to call this here?
            
        scenes[name]->setActive(active);
        scenes[name]->setPaused(paused);
    }
}

void Game::toggleFullscreen()
{
    if (isFullscreen) 
    {
        // go back to windowed mode
        SetWindowSize(lastWindowW, lastWindowH);
        SetWindowPosition(lastWindowX, lastWindowY);
        ShowCursor();
    }
    else 
    {
        // some manual adjustment is needed here because raylib doesn't handle changing aspect ratios well
        // Store current windowed size and position
        lastWindowW = GetScreenWidth();
        lastWindowH = GetScreenHeight();
        auto [x, y] = GetWindowPosition();
        lastWindowX = static_cast<int>(x);
        lastWindowY = static_cast<int>(y);

        // Set to fullscreen size
        int width = GetMonitorWidth(0);
        int height = GetMonitorHeight(0);
        SetWindowSize(width, height);
        HideCursor();
    }
    isFullscreen = !isFullscreen;
    writeSetting("fullscreen", isFullscreen);
    ToggleFullscreen();
}

void Game::toggleVsync()
{
    if (IsWindowState(FLAG_VSYNC_HINT))
    {
        ClearWindowState(FLAG_VSYNC_HINT);
        writeSetting("vsync", false);
    }
    else
    {
        SetWindowState(FLAG_VSYNC_HINT);
        writeSetting("vsync", true);
    }
}

void Game::sleepScene(const std::string& name) { setSceneState(name, false, false); }
void Game::wakeScene(const std::string& name) { setSceneState(name, true, false); }
void Game::pauseScene(const std::string& name) { setSceneState(name, true, true); }
void Game::resumeScene(const std::string& name) { setSceneState(name, true, false); }

void Game::resetScenes() 
{
    for (const auto& [name, scene] : scenes) 
    {
        stopScene(name);
    }
}

void Game::processMarkedScenes() 
{
    for (auto it = scenes.begin(); it != scenes.end(); ) 
    {
        if (it->second->ismarkedForStarting())
        {
            TraceLog(LOG_INFO, "starting scene %s", it->second->getName().c_str());
            it->second->markedForStarting = false;
            it->second->startup();
            it->second->setActive(true);
        }
        else if (it->second->isMarkedForDeletion())
        {
            it->second->end();
            it = scenes.erase(it);
        } 
        else
        {
            ++it;
        }
    }
}

void Game::save(std::string& filename)
{
    // saves the game data to a JSON file
    SaveGame save; // create a new save game object
    // TODO take the existing save game object
    // TODO split these into two methods (write to SaveGame object, and serialize and save the SaveGame object)

    save.playerMaxHealth = getPlayer()->maxHealth;
    save.playerHealth = std::max(static_cast<uint32_t>(6), getPlayer()->health); // ensure the player always starts with at least 3 hearts
    InGame* inGame = dynamic_cast<InGame*>(getScene("InGame"));
    assert(inGame && "InGame scene not accessible");
    if (inGame->currentWeapon[0])
        save.currentWeapons[0] = inGame->currentWeapon[0]->first;
    if (inGame->currentWeapon[1])
        save.currentWeapons[1] = inGame->currentWeapon[1]->first;

    for (auto& sprite : sprites)
    {
        // check if a sprite follows the player.
        // That sprite should be spawned again after loading
        if (sprite->followsPlayer && sprite->persistent)
            save.spritesFollowingPlayer.push_back(sprite->spriteName);
    }

    save.items = {}; 
    auto& invItems = inventory.getItems();
    for (size_t type = 0; type < NUM_ITEM_TYPES; type++)
    {
        for (auto& item : invItems[type])
        {
            save.items.push_back({ item.first, item.second.second }); // key, quantity
        }
    }

    // add the current world data
    saveWorld(save, *currentWorld);

    // add data from the current savegame object about previously visited worlds
    // TODO just a temporary fix
    for (auto& it : savegame->worldData)
    {
        if (it.first != currentWorld->name)
        {
            save.worldData[it.first] = it.second;
        }
    }

    // save the last visited world
    save.lastWorld = currentWorld->name;

    auto j = writeDataToJSON(save);

    std::ofstream file("./savegames/" + filename + ".json");
    file << j.dump(2);
    TraceLog(LOG_INFO, "The game was saved to %s.", filename.c_str());

    // save a screenshot for the loading menu thumbnail
    if (!this->lastScreenshot)
        return;

    Image img = ImageCopy(*this->lastScreenshot);
    ImageFlipVertical(&img);
    // TODO: image looks blurry
    int w = static_cast<int>(gameScreenWidth / 2);
    int h = static_cast<int>(gameScreenHeight / 2);
    ImageResize(&img, w, h);
    std::string path = "savegames/thumbs/" + filename + ".png";
    ExportImage(img, path.c_str());
    UnloadImage(img);
}

void Game::load(std::string& filename)
{
    std::ifstream fileStream("./savegames/" + filename + ".json");
    nlohmann::json jsonData;
    fileStream >> jsonData;

    savegame = std::make_shared<SaveGame>(readSaveDataFromJSON(jsonData));
    // Unpacking the savegame object happens in InGame.cpp at startup
    eventManager.pushEvent(LOADING_SAVEGAME_SUCCESS);
}

std::shared_ptr<SaveGame> Game::getSaveData()
{
    return savegame;
}

int Game::getGamepadButtonForControl(uint32_t control)
{
    auto it = controlGamepadMap.find(control);
    if (it != controlGamepadMap.end() && !it->second.empty())
        return it->second[0];

    return -1;
}

int Game::getKeyForControl(uint32_t control)
{
    auto it = controlKeyMap.find(control);
    if (it != controlKeyMap.end() && !it->second.empty())
        return it->second[0];

    return -1;
}
std::shared_ptr<Sprite> Game::createSprite(std::string spriteName, Rectangle& rect)
{
    auto sprite = std::make_shared<Sprite>(*this, rect.x, rect.y, rect.width, rect.height, spriteName);
    // delay the addition to the sprites vector until the end of the loop
    spritesToAdd.push_back(sprite);
    return sprite;
}

void Game::createWorld(const std::string& key, bool isDungeon)
{
    const auto& allWorldsData = loader.getDungeonData();
    if (!allWorldsData.contains(key))
        throw std::runtime_error("Dungeon '" + key + "' not found in dungeons.json");

    const auto& worldData = allWorldsData[key];

    size_t roomsW = worldData["rooms_w"];
    size_t roomsH = worldData["rooms_h"];
    size_t numLevels = worldData["num_levels"];

    if (isDungeon)
    {
        auto dungeon = std::make_unique<Dungeon>(*this, roomsW, roomsH, numLevels, key);
        dungeon->generate(worldData);
        dungeon->makeMapTextures();
        currentWorld = std::move(dungeon);
    }
    else
    {
        // load overworld data
        auto overworld = std::make_unique<Overworld>(*this, roomsW, roomsH, numLevels, key);
        overworld->generate(worldData);
        overworld->makeMapTextures();
        currentWorld = std::move(overworld);
    }
}

void Game::killSprite(const std::shared_ptr<Sprite>& sprite) 
{
    auto it = std::find(sprites.begin(), sprites.end(), sprite);
    if (it != sprites.end())
    {
        *it = std::move(sprites.back());
        sprites.pop_back();
    }
}

void Game::clearSprites(bool clearPersistent) 
{
    // removes all current sprites
    // keeps the ones with the "persistent" flag, if not stated otherwise
    for (auto& sprite: sprites)
    {
        if (!sprite->persistent)
        {
            sprite->markForDeletion();
            TraceLog(LOG_INFO, "deleting Sprite %s", sprite->spriteName.c_str());
        }
    }
}

void Game::processMarkedSprites() 
{
    // Remove marked sprites from spriteMap first
    for (auto& sprite : sprites)
    {
        if (sprite->isMarkedForDeletion())
        {
            auto it = spriteMap.find(sprite->spriteName);
            if (it != spriteMap.end() && it->second == sprite)
                spriteMap.erase(it);
        }
    }

    // Remove marked sprites from sprites vector
    sprites.erase(std::remove_if(sprites.begin(), sprites.end(),
        [](auto sprite) {
            return sprite->isMarkedForDeletion();
        }), sprites.end());

    // add any new sprites to the vector
    for (auto& s : spritesToAdd)
    {
        sprites.push_back(s);
    }
    spritesToAdd.clear();
}

void Game::clearEmitters(bool clearPersistent)
{
    emitters.erase(std::remove_if(emitters.begin(), emitters.end(),
        [](auto emitter) {
            return !emitter->persistent;
        }), emitters.end());
}

Sprite* Game::getPlayer() 
{
    InGame* inGame = dynamic_cast<InGame*>(getScene("InGame"));
    if (!inGame)
        return nullptr;
    auto it = spriteMap.find("player");
    if (it != spriteMap.end())
        return it->second.get();
    return nullptr;
}

void Game::playSound(const std::string& key)
{
    if (!soundOn || !sfxOn)
        return;
    Sound sound = loader.getSound(key);
    SetSoundVolume(sound, getSetting<float>("sfxVolume") / 100.0f);
    PlaySound(sound);
}

void Game::update(float deltaTime)
{
    eventManager.update(deltaTime);
    windowEvents.update();

    for (auto& [name, scene] : scenes)
    {
        if (scene && scene->isActive() && !scene->isPaused())
        {
            scene->update(deltaTime);
        }
    }
}

void Game::playMusic()
{
    if (!soundOn || !musicOn)
        return;

    for (auto& [name, scene] : scenes)
    {
        if (scene && scene->isActive())
        {
            if (scene->music)
            {
                SetMusicVolume(*scene->music, getSetting<float>("musicVolume") / 100.0f);
                UpdateMusicStream(*scene->music);
            }
        }
    }
}

void Game::draw()
{
    // Compute required framebuffer scaling
    float scale = std::min((float)GetScreenWidth() / gameScreenWidth, (float)GetScreenHeight() / gameScreenHeight);
    
    // Draw everything in the render texture, note this will not be rendered on screen, yet
    // All the actual drawing logic is handled by each scene
    BeginTextureMode(target);
        std::vector<Scene*> activeScenes;
        for (auto& [name, scene] : scenes)
        {
            if (scene && scene->isActive())
                activeScenes.push_back(scene.get());
        }
        // Sort active scenes by draw priority
        std::sort(activeScenes.begin(), activeScenes.end(),
            [](Scene* a, Scene* b) {
                return a->getDrawPriority() < b->getDrawPriority();
            });
        for (Scene* scene : activeScenes)
        {
            scene->draw();
        }
    EndTextureMode();

    // now draw "target" onto the actual window
    BeginDrawing();
        ClearBackground(BLACK);
        // Draw render texture to screen, properly scaled
        DrawTexturePro(
            target.texture, 
            Rectangle{ 
                0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height 
            },
            Rectangle{
                (GetScreenWidth() - ((float)gameScreenWidth * scale)) * 0.5f,
                (GetScreenHeight() - ((float)gameScreenHeight * scale)) * 0.5f,
                (float)gameScreenWidth* scale, 
                (float)gameScreenHeight* scale
            }, 
            Vector2{ 0, 0 }, 
            0.0f,
            WHITE);

        if (debug)
        {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.25f));
            // show the state of the scenes
            int fontSize = 24;

            std::string s_activeScenes = "Scenes active:\n";
            std::string s_pausedScenes = "Scenes paused:\n";
            std::string s_inactiveScenes = "Scenes inactive:\n";
            std::string s_drawOrder = "Draw Order:\n";

            for (auto& [name, scene] : scenes)
            {
                std::string stateInfo = " (" + std::to_string(static_cast<int>(scene->getState())) + ")";
                if (scene && scene->isActive() && !scene->isPaused())
                {
                    s_activeScenes += scene->getName() + stateInfo + "\n";
                }
                else if (scene && scene->isActive() && scene->isPaused())
                {
                    s_pausedScenes += scene->getName() + stateInfo + "\n";
                }
                else if (scene && !scene->isActive())
                {
                    s_inactiveScenes += scene->getName() + stateInfo + "\n";
                }
            }
            for (Scene* scene : activeScenes)
            {
                s_drawOrder += scene->getName() + " (Priority: " + std::to_string(scene->getDrawPriority()) + ")\n";
            }

            DrawText(s_activeScenes.c_str(), 4, 4, fontSize, WHITE);
            DrawText(s_pausedScenes.c_str(), int(GetScreenWidth() * 0.3f), 4, fontSize, WHITE);
            DrawText(s_inactiveScenes.c_str(), int(GetScreenWidth() * 0.6f), 4, fontSize, WHITE);
            DrawText(s_drawOrder.c_str(), 4, int(GetScreenHeight() * 0.6f), fontSize, WHITE); // lower part of screen

            if (currentWorld)
            {
                size_t maxIndex = currentWorld->getSize().first * currentWorld->getSize().second;
                if (currentWorld->currentRoomIndex < maxIndex)
                {
                    std::ostringstream ss;

                    size_t roomIndex = currentWorld->currentRoomIndex;

                    Room* room = currentWorld->getCurrentRoom();
                    if (room)
                    {
                        std::string roomName = room->tilemap.getName();
                        uint8_t roomState = room->state;
                        ss << "Room: " << roomName << " (" << roomIndex << "); state: " << int(roomState);
                    }
                    else
                    {
                        ss << "no room at index " << roomIndex;
                    }
                    DrawText(ss.str().c_str(), 4, int(GetScreenHeight() * 0.8f), fontSize, WHITE);
                }
                else
                {
                    DrawText("invalid room index", 4, int(GetScreenHeight() * 0.8f), fontSize, WHITE);
                }
            }

            // visualize player position
            Sprite* p = getPlayer();
            if (p)
            {
                Vector2 playerPos = p->position;
                DrawText(format("Player pos: (%d, %d)", int(playerPos.x), int(playerPos.y)).c_str(), 4, int(GetScreenHeight() * 0.85f), fontSize, WHITE);
            }
        }
    EndDrawing();
}

void Game::run()
{
    float lastTime = (float)GetTime();
    char title[64];

    // start the first scene
    startScene("Preload");
    // enable saving the game state from any scene
    eventManager.addListener(SAVE_GAME, [&](std::any data) {
        std::string filename = std::any_cast<std::string>(data);
        save(filename);
        playSound("Rise02");
        });
    // loading a saved game
    eventManager.addListener(LOAD_GAME, [&](std::any data) {
        std::string str = std::any_cast<std::string>(data);
        load(str);
        });
    // main game loop
    while (running && !WindowShouldClose())
    {
        // show FPS in title
        snprintf(title, sizeof(title), "My Game - FPS: %d", GetFPS());
        SetWindowTitle(title);
        // get the recently pressed/held down buttons
        buttonsPressed = GetControlsPressed();
        buttonsDown = GetControlsDown();
   
        // debug mode toggle
        if (buttonsPressed & CONTROL_DEBUG)
            debug = !debug; 
        // specific debug functions
        if (debug)
        {
            if (buttonsPressed & CONTROL_DEBUG_K2)
                soundOn = !soundOn;
        }

        // --- main game loop ---
        float currentTime = float(GetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        update(deltaTime);
        playMusic();
        draw();

        // toggle fullscreen
        if (IsKeyPressed(KEY_F4))
            toggleFullscreen();

        // restart the game 
        // TODO: this is just for faster debugging, will be removed in the final version
        if (IsKeyPressed(KEY_F5))
            restart();

        // safely clean up sprites and scenes after the other logic is done
        processMarkedSprites();
        processMarkedScenes();
    }
    // cleanup after the game loop
    cleanup();
}
