#include "Game.h"
#include <raylib.h>
#include "Preload.h"
#include "TitleScreen.h"
#include "InGame.h"
#include "HUD.h"
#include "GameOver.h"
#include "Utils.h"


Game::Game() : buttonsDown{}, buttonsPressed{} {
    // Enable config flags for resizable window and vsync
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(windowWidth, windowHeight, "my first game");
    SetWindowMinSize(320, 240);

    // Render texture initialization, used to hold the rendering result so we can easily resize it
    // see https://github.com/raysan5/raylib/blob/master/examples/core/core_window_letterbox.c
    target = LoadRenderTexture(gameScreenWidth, gameScreenHeight);

    SetTargetFPS(120);

    // define all Scenes as factory functions
    registerScene<Preload>("Preload");
    registerScene<TitleScreen>("TitleScreen");
    registerScene<InGame>("InGame");
    registerScene<HUD>("HUD");
    registerScene<GameOver>("GameOver");
}

void Game::startScene(const std::string& name) {   
    if (sceneRegistry.count(name)) {
        scenes[name] = sceneRegistry[name](name);
        scenes[name]->markForStarting();
    }
    else {
        Log("Scene not registered: " + name);
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
            it->second->startup(this);
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

Sprite* Game::getPlayer() {
    InGame* inGame = dynamic_cast<InGame*>(getScene("InGame"));
    if (!inGame) return nullptr;
    auto it = inGame->spriteMap.find("player");
    if (it != inGame->spriteMap.end()) return it->second.get();
    return nullptr;
}

void Game::events() {
    for (auto& [name, scene] : scenes) {
        if (scene && scene->isActive() && !scene->isPaused()) {
            scene->events(this);
        }
    }
}

void Game::update(float dt) {
    eventManager.update(dt);

    for (auto& [name, scene] : scenes) {  // Iterate over map
        if (scene && scene->isActive() && !scene->isPaused()) {
            scene->update(this, dt);
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
                scene->draw(this);
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
        events();
        update(dt);
        draw();

        processMarkedScenes();
    }

    // cleanup
    UnloadRenderTexture(target);
    CloseWindow();
}
