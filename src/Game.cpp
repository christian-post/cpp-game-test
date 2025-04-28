#include "Game.h"
//#include <raylib.h>
#include "Controls.h"
#include "Preload.h"
#include "TitleScreen.h"
#include "InGame.h"
#include "HUD.h"
#include "Inventory.h"
#include "GameOver.h"
#include "Utils.h"


Game::Game() : buttonsDown{}, buttonsPressed{} {
    loader.loadSettings("./resources/settings.json");
    settings = &loader.getSettings();
    TraceLog(LOG_INFO, settings->dump(2).c_str());
    // Enable config flags for resizable window and vsync
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    //InitWindow(windowWidth, windowHeight, "my first game");
    InitWindow(getSetting("windowWidth"), getSetting("windowHeight"), "My first game");
    SetWindowMinSize(320, 240);

    // Render texture initialization, used to hold the rendering result so we can easily resize it
    // see https://github.com/raysan5/raylib/blob/master/examples/core/core_window_letterbox.c
    gameScreenWidth = getSetting("gameScreenWidth");
    gameScreenHeight = getSetting("gameScreenHeight");
    target = LoadRenderTexture(gameScreenWidth, gameScreenHeight);

    SetTargetFPS(getSetting("targetFPS"));

    // define all Scenes as factory functions
    registerScene<Preload>("Preload");
    registerScene<TitleScreen>("TitleScreen");
    registerScene<InGame>("InGame");
    registerScene<HUD>("HUD");
    registerScene<Inventory>("Inventory");
    registerScene<GameOver>("GameOver");
}

const nlohmann::json& Game::getSetting(const std::string& key) const {
    try {
        return settings->at(key);
    }
    catch (const std::out_of_range&) {
        std::cerr << "[Settings] Missing key: " << key << "\n";
        std::terminate(); // or throw
    }
}

void Game::startScene(const std::string& name) {   
    if (sceneRegistry.count(name)) {
        scenes[name] = sceneRegistry[name](name);
        scenes[name]->markForStarting();
    }
    else {
        TraceLog(LOG_ERROR, "Scene not registered: %s", name.c_str());
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
    sprites.erase(
        std::remove_if(sprites.begin(), sprites.end(),
            [clearPersistent](const std::shared_ptr<Sprite>& s) {
                return clearPersistent || !s->persistent;
            }),
        sprites.end()
    );
}

Sprite* Game::getPlayer() {
    InGame* inGame = dynamic_cast<InGame*>(getScene("InGame"));
    if (!inGame) return nullptr;
    auto it = inGame->spriteMap.find("player");
    if (it != inGame->spriteMap.end()) return it->second.get();
    return nullptr;
}

void Game::update(float dt) {
    eventManager.update(dt);

    for (auto& [name, scene] : scenes) {  // Iterate over map
        if (scene && scene->isActive() && !scene->isPaused()) {
            scene->update(dt);
        }
    }
}

void Game::draw() {
    // Compute required framebuffer scaling
    float scale = std::min((float)GetScreenWidth() / gameScreenWidth, (float)GetScreenHeight() / gameScreenHeight);
    
    // Draw everything in the render texture, note this will not be rendered on screen, yet
    // All the actual drawing logic is handled by each scene
    BeginTextureMode(target);
        for (auto& [name, scene] : scenes) {  // Iterate over map
            if (scene && scene->isActive()) {
                scene->draw();
            }
        }
    EndTextureMode();

    // now draw "target" onto the actual window
    BeginDrawing();
        ClearBackground(BLACK);     // Clear screen background

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
            // show the state of the scenes
            int fontSize = 18;

            std::string s_activeScenes = "Scenes active:\n";
            std::string s_pausedScenes = "Scenes paused:\n";
            std::string s_inactiveScenes = "Scenes inactive:\n";
            for (auto& [name, scene] : scenes) {
                if (scene && scene->isActive() && !scene->isPaused()) {
                    s_activeScenes += scene->getName() + "\n";
                }
                else if (scene && scene->isActive() && scene->isPaused()) {
                    s_pausedScenes += scene->getName() + "\n";
                }
                else if (scene && !scene->isActive()) {
                    s_inactiveScenes += scene->getName() + "\n";
                }
            }
            DrawText(s_activeScenes.c_str(), 4, 4, fontSize, WHITE);
            DrawText(s_pausedScenes.c_str(), int(GetScreenWidth() * 0.3f), 4, fontSize, WHITE);
            DrawText(s_inactiveScenes.c_str(), int(GetScreenWidth() * 0.6f), 4, fontSize, WHITE);
        }
    EndDrawing();
}

void Game::run() {
    float lastTime = (float)GetTime();
    char title[64];

    // start the first scene
    startScene("Preload");

    while (running && !WindowShouldClose()) {
        // show FPS in title
        snprintf(title, sizeof(title), "My Game - FPS: %d", GetFPS());
        SetWindowTitle(title);

        buttonsPressed = GetControlsPressed();
        buttonsDown = GetControlsDown();
        // debug mode toggle
        if (buttonsPressed & CONTROL_DEBUG) debug = !debug;

        float currentTime = float(GetTime());
        float dt = currentTime - lastTime;
        lastTime = currentTime;
        update(dt);
        draw();

        processMarkedScenes();
    }

    // cleanup
    UnloadRenderTexture(target);
    CloseWindow();
}
