#include "Game.h"
#include "Controls.h"
#include "Preload.h"
#include "TitleScreen.h"
#include "StartMenu.h"
#include "SelectMenu.h"
#include "InGame.h"
#include "HUD.h"
#include "InventoryUI.h"
#include "MapUI.h"
#include "GameOver.h"
#include "Utils.h"
#include <sstream>
#include <fstream>


Game::Game() : buttonsDown{}, buttonsPressed{}, inventory(*this) {
    loader.loadSettings("./resources/settings.json");
    settings = &loader.getSettings();
    TraceLog(LOG_INFO, settings->dump(2).c_str());
    // Enable config flags for resizable window and vsync
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(getSetting("windowWidth"), getSetting("windowHeight"), "My first game");
    SetWindowMinSize(320, 240);
    InitAudioDevice();

    soundOn = getSetting("soundOn");

    // Render texture initialization, used to hold the rendering result so we can easily resize it
    // see https://github.com/raysan5/raylib/blob/master/examples/core/core_window_letterbox.c
    gameScreenWidth = getSetting("gameScreenWidth");
    gameScreenHeight = getSetting("gameScreenHeight");
    target = LoadRenderTexture(gameScreenWidth, gameScreenHeight);

    SetTargetFPS(getSetting("targetFPS"));

    // define all Scenes as factory functions
    // the second argument is priority for the drawing order
    registerScene<Preload>("Preload", 0);
    registerScene<TitleScreen>("TitleScreen", 0);
    registerScene<StartMenu>("StartMenu", 0);
    registerScene<SelectMenu>("SelectMenu", 0);
    registerScene<InGame>("InGame", 0);
    registerScene<HUD>("HUD", 1);
    registerScene<InventoryUI>("InventoryUI", 2);
    registerScene<MapUI>("MapUI", 2);
    registerScene<GameOver>("GameOver", 2);
}

const nlohmann::json& Game::getSetting(const std::string& key) const {
    try {
        return settings->at(key);
    }
    catch (const std::out_of_range&) {
        TraceLog(LOG_ERROR, "[Settings] Missing key: %s", key.c_str());
        std::terminate();
    }
}

void Game::startScene(const std::string& name) {
    if (sceneRegistry.count(name)) {
        scenes[name] = sceneRegistry[name](name);

        if (scenePriorities.count(name))
            scenes[name]->setDrawPriority(scenePriorities[name]);
        else
            scenes[name]->setDrawPriority(0); // default

        scenes[name]->markForStarting();
    }
    else {
        TraceLog(LOG_ERROR, "Scene not registered: %s", name.c_str());
        throw std::invalid_argument("No scene with this name. See console for details.");
    }
}


void Game::stopScene(const std::string& name) {
    // Marks the scene for removal
    if (scenes.count(name)) {
        scenes[name]->markForDeletion();  
    }
}

void Game::setSceneState(const std::string& name, bool active, bool paused) {
    if (scenes.count(name)) {  // Check if the scene exists
        scenes[name]->setActive(active);
        scenes[name]->setPaused(paused);
    }
}

void Game::sleepScene(const std::string& name) { setSceneState(name, false, false); }
void Game::wakeScene(const std::string& name) { setSceneState(name, true, false); }
void Game::pauseScene(const std::string& name) { setSceneState(name, true, true); }
void Game::resumeScene(const std::string& name) { setSceneState(name, true, false); }

void Game::resetScenes() {
    for (const auto& [name, scene] : scenes) {
        stopScene(name);
    }
}

void Game::processMarkedScenes() {
    for (auto it = scenes.begin(); it != scenes.end(); ) {
        if (it->second->ismarkedForStarting()) {
            it->second->markedForStarting = false;
            it->second->startup();
            it->second->setActive(true);
        }
        else if (it->second->isMarkedForDeletion()) {
            it->second->end();
            it = scenes.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::save()
{
    // saves the game data to a JSON file
    SaveGame save; // create a new save game object

    save.playerMaxHealth = getPlayer()->maxHealth;
    save.playerHealth = std::max(static_cast<uint32_t>(6), getPlayer()->health); // ensure the player always starts with at least 3 hearts
    save.items = {}; 
    save.DungeonRooms = {};

    auto& invItems = inventory.getItems();
    for (size_t type = 0; type < NUM_ITEM_TYPES; type++) {
        for (auto item : invItems[type]) {
            save.items.push_back({ item.first, item.second.second }); // key, quantity
        }
    }

    // serialize the dungeon room data
    saveDungeon(save, *currentDungeon);

    auto j = writeDataToJSON(save);

    //j["DungeonRooms"] = {}; // TODO: temporary fix

    // TODO testing
    std::ofstream file("./savegames/save_0.json");
    file << j.dump(2);

    TraceLog(LOG_INFO, "The game was saved to save_0.");
}

void Game::load()
{
    TraceLog(LOG_INFO, "Available Save Files:");
    std::vector<std::string> files = listJSONFiles("./savegames");
    for (auto file : files)
        TraceLog(LOG_INFO, file.c_str());

    if (!files.empty()) {
        std::ifstream fileStream(files[0]);
        nlohmann::json jsonData;
        fileStream >> jsonData;
        // TODO: print the data for debugging
        TraceLog(LOG_INFO, jsonData.dump(2).c_str());

        savegame = std::make_shared<SaveGame>(readSaveDataFromJSON(jsonData));
        eventManager.pushEvent("loadingSavegameSuccess");
    }
}

std::shared_ptr<SaveGame> Game::getSaveData()
{
    return savegame;
}

std::shared_ptr<Sprite> Game::createSprite(std::string spriteName, Rectangle& rect)
{
    auto sprite = std::make_shared<Sprite>(
        *this, rect.x, rect.y, rect.width, rect.height, spriteName
    );
    // delay the addition to the sprites vector until the end of the loop
    spritesToAdd.push_back(sprite);
    return sprite;
}

void Game::createDungeon(size_t roomsW, size_t roomsH)
{
    currentDungeon = std::make_unique<Dungeon>(*this, roomsW, roomsH);
    // TODO: hardcoding this here for now
    /*
    ## test dungeon ##
       0   1   2   3
    1  x 006   x   x
    2  x 003 002   x
    3  x 004 001 005
    */
    // coordinates are row, column
    // second argument is the directions of the doors, starting at the right and going counter clockwise
     
#ifdef TEST_ROOM
    currentDungeon->insertRoom(0, 0, Room{ loader.getTilemap("test_map_small"), 0b0000 }); // test dungeon
#else
    currentDungeon->insertRoom(3, 2, Room{ loader.getTilemap("dungeon001"), 0b1111 });
    currentDungeon->insertRoom(2, 2, Room{ loader.getTilemap("dungeon002"), 0b0011 });
    currentDungeon->insertRoom(2, 1, Room{ loader.getTilemap("dungeon003"), 0b1001 });
    currentDungeon->insertRoom(3, 1, Room{ loader.getTilemap("dungeon004"), 0b1100 });
    currentDungeon->insertRoom(3, 3, Room{ loader.getTilemap("dungeon005"), 0b0010 });
    currentDungeon->insertRoom(1, 1, Room{ loader.getTilemap("dungeon006"), 0b0101 });
#endif // TEST_ROOM

    // TODO: debugging, setting all rooms to visited
    //for (int i = 0; i < roomsW * roomsH; i++) {
    //    currentDungeon->setVisited(i);
    //}

#ifdef TEST_ROOM
    currentDungeon->setCurrentRoomIndex(0); // TODO: testing
#else 
    currentDungeon->setCurrentRoomIndex(14); // start in R1
#endif // TEST_ROOM

    currentDungeon->makeMinimapTextures();

    // TODO
    // set the player's starting position correctly
}

void Game::killSprite(const std::shared_ptr<Sprite>& sprite) {
    auto it = std::find(sprites.begin(), sprites.end(), sprite);
    if (it != sprites.end()) {
        *it = std::move(sprites.back());
        sprites.pop_back();
    }
}

void Game::clearSprites(bool clearPersistent) {
    // removes all current sprites
    // keeps the ones with the "persistent" flag, if not stated otherwise
    // TODO: testing delayed removal
    for (auto sprite: sprites) {
        if (!sprite->persistent) {
            sprite->markForDeletion();
            TraceLog(LOG_INFO, "deleting Sprite %s", sprite->spriteName.c_str());
        }
    }
}

void Game::processMarkedSprites() {
    sprites.erase(std::remove_if(sprites.begin(), sprites.end(),
        [](auto sprite) {
            return sprite->isMarkedForDeletion();
        }), sprites.end());

    // add any new sprites to the vector
    // TODO: just doing this here, no need for a seperate function I guess
    for (auto s : spritesToAdd) {
        sprites.push_back(s);
    }
    spritesToAdd.clear();
}

Sprite* Game::getPlayer() {
    InGame* inGame = dynamic_cast<InGame*>(getScene("InGame"));
    if (!inGame) return nullptr;
    auto it = inGame->spriteMap.find("player");
    if (it != inGame->spriteMap.end()) return it->second.get();
    return nullptr;
}

void Game::playSound(const std::string& key){
    if (!soundOn || !sfxOn) return;
    PlaySound(loader.getSound(key));
}

void Game::update(float deltaTime) {
    eventManager.update(deltaTime);

    for (auto& [name, scene] : scenes) {
        if (scene && scene->isActive() && !scene->isPaused()) {
            scene->update(deltaTime);
        }
    }
}

void Game::playMusic() {
    if (!soundOn || !musicOn) return;
    for (auto& [name, scene] : scenes) {
        if (scene && scene->isActive()) {
            if (scene->music) {
                UpdateMusicStream(*scene->music);
                //return; // TODO prevent multiple scenes from playing music?
            }
        }
    }
}

void Game::draw() {
    // Compute required framebuffer scaling
    float scale = std::min((float)GetScreenWidth() / gameScreenWidth, (float)GetScreenHeight() / gameScreenHeight);
    
    // Draw everything in the render texture, note this will not be rendered on screen, yet
    // All the actual drawing logic is handled by each scene
    BeginTextureMode(target);
        std::vector<Scene*> activeScenes;
        for (auto& [name, scene] : scenes) {
            if (scene && scene->isActive()) {
                activeScenes.push_back(scene.get());
            }
        }
        // Sort active scenes by draw priority
        std::sort(activeScenes.begin(), activeScenes.end(),
            [](Scene* a, Scene* b) {
                return a->getDrawPriority() < b->getDrawPriority();
            });
        for (Scene* scene : activeScenes) {
            scene->draw();
        }
    EndTextureMode();

    // now draw "target" onto the actual window
    BeginDrawing();
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

        if (debug) {
            DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.25f));
            // show the state of the scenes
            int fontSize = 24;

            std::string s_activeScenes = "Scenes active:\n";
            std::string s_pausedScenes = "Scenes paused:\n";
            std::string s_inactiveScenes = "Scenes inactive:\n";
            std::string s_drawOrder = "Draw Order:\n";

            for (auto& [name, scene] : scenes) {
                std::string stateInfo = " (" + std::to_string(static_cast<int>(scene->getState())) + ")";
                if (scene && scene->isActive() && !scene->isPaused()) {
                    s_activeScenes += scene->getName() + stateInfo + "\n";
                }
                else if (scene && scene->isActive() && scene->isPaused()) {
                    s_pausedScenes += scene->getName() + stateInfo + "\n";
                }
                else if (scene && !scene->isActive()) {
                    s_inactiveScenes += scene->getName() + stateInfo + "\n";
                }
            }
            for (Scene* scene : activeScenes) {
                s_drawOrder += scene->getName() + " (Priority: " + std::to_string(scene->getDrawPriority()) + ")\n";
            }

            DrawText(s_activeScenes.c_str(), 4, 4, fontSize, WHITE);
            DrawText(s_pausedScenes.c_str(), int(GetScreenWidth() * 0.3f), 4, fontSize, WHITE);
            DrawText(s_inactiveScenes.c_str(), int(GetScreenWidth() * 0.6f), 4, fontSize, WHITE);
            DrawText(s_drawOrder.c_str(), 4, int(GetScreenHeight() * 0.6f), fontSize, WHITE); // lower part of screen

            // TODO: create another function to get the current Tilemap data that doesn't log constantly on error
            size_t maxIndex = currentDungeon->getSize().first * currentDungeon->getSize().second;
            if (currentDungeon->getCurrentRoomIndex() < maxIndex) {
                std::ostringstream ss;
                ss << "Room: " << currentDungeon->loadCurrentTileMap()->getName()
                    << " (" << currentDungeon->getCurrentRoomIndex() << ")";
                DrawText(ss.str().c_str(), 4, int(GetScreenHeight() * 0.8f), fontSize, WHITE);
            }
            else {
                DrawText("invalid room index", 4, int(GetScreenHeight() * 0.8f), fontSize, WHITE);
            }
        }
    EndDrawing();
}

void Game::run() {
    float lastTime = (float)GetTime();
    char title[64];

    // start the first scene
    startScene("Preload");
    // enable saving the game state from any scene
    eventManager.addListener("saveGame", [&](const std::any& data) {
        save();
        });
    // loading a saved game
    // TODO: use data for the file index
    eventManager.addListener("loadGame", [&](const std::any& data) {
        load();
        });
    
    while (running && !WindowShouldClose()) {
        // show FPS in title
        snprintf(title, sizeof(title), "My Game - FPS: %d", GetFPS());
        SetWindowTitle(title);
        // get the recently pressed/held down buttons
        buttonsPressed = GetControlsPressed();
        buttonsDown = GetControlsDown();
   
        if (buttonsPressed & CONTROL_DEBUG) debug = !debug; // debug mode toggle
        // specific debug functions
        if (debug) {
            if (buttonsPressed & CONTROL_DEBUG_K2) {
                soundOn = !soundOn;
            }
        }

        float currentTime = float(GetTime());
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        update(deltaTime);
        playMusic();
        draw();

        // restarting the game (debugging)
        if (IsKeyPressed(KEY_F5)) {
            restart();
        }
        processMarkedSprites();
        processMarkedScenes();
    }
    // cleanup after the game loop
    UnloadRenderTexture(target);
    CloseAudioDevice();
    CloseWindow();
}
