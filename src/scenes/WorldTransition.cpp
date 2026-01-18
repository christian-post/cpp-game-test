#include "WorldTransition.h"
#include "Game.h"
#include "CircleOverlay.h"

WorldTransition::WorldTransition(Game& game, const std::string& name)
    : Scene(game, name)
{
    game.eventManager.addListener(CAMERA_UPDATE, [this](std::any data) {
        // get data about the camera and its target to position the transition effect correctly
        auto [cam, playerPos] = std::any_cast<std::pair<Camera2D&, Vector2>>(data);
        this->playerScreenPosition = GetWorldToScreen2D(playerPos, cam);
        });
}

void WorldTransition::startup()
{
    game.eventManager.pushEvent(LOCK_PLAYER_MOVEMENT);
    state = TransitionState::CLOSING; // ensure the state is CLOSING (== visually "closing" the previous scene) at the start
    closingStartTime = GetTime();
}

void WorldTransition::update(float deltaTime)
{
    switch (state)
    {
    case TransitionState::CLOSING:
        timer += deltaTime;
        if (timer > animDuration)
        {
            timer = 0.0f;
            state = TransitionState::LOADING; // start loading
        }
        break;
    case TransitionState::OPENING:
        timer += deltaTime;
        if (timer > animDuration)
        {
            timer = 0.0f;
            game.eventManager.pushEvent(UNLOCK_PLAYER_MOVEMENT);
            game.stopScene(getName());
        }
        break;
    case TransitionState::LOADING:
        // empty the load queue
        if (!game.loader.loadQueue.empty()) {
            //currentMessage = loadQueue.front().first; // TODO should I display this here?
            game.loader.loadQueue.front().second(); // execute callback
            game.loader.loadQueue.pop();
        }
        else {
            state = TransitionState::OPENING;
            closingStartTime = GetTime();
        }
        break;
    }
}

void WorldTransition::draw()
{
    switch (state)
    {
    case TransitionState::CLOSING:
        DrawTransition(game.target.texture, game.loader.getShader("circle_transition"), playerScreenPosition, static_cast<float>(game.gameScreenWidth), static_cast<float>(game.gameScreenHeight), closingStartTime, animDuration, 0);
        break;
    case TransitionState::OPENING:
        DrawTransition(game.target.texture, game.loader.getShader("circle_transition"), playerScreenPosition, static_cast<float>(game.gameScreenWidth), static_cast<float>(game.gameScreenHeight), closingStartTime, animDuration, 1);
        break;
    case TransitionState::LOADING:
        // all black with text
        ClearBackground(BLACK);

        const char* text = "Loading...";
        int fontSize = 12;
        int margin = 24;
        // put the text at the bottom right
        int textWidth = MeasureText(text, fontSize);
        int x = game.gameScreenWidth - textWidth - margin;
        int y = game.gameScreenHeight - fontSize - margin;
        DrawText(text, x, y, fontSize, LIGHTGRAY);

        break;
    }
}

void WorldTransition::end()
{
    game.eventManager.removeListeners(CAMERA_UPDATE);
}
