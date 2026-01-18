#pragma once
#include "Scene.h"
#include <iostream>

enum class TransitionState
{
    OPENING,
    LOADING,
    CLOSING
};

class WorldTransition : public Scene 
{
    // This scene plays when the player goes from one world to another
    // It shows a transition animation that hides the short delay due to loading
    // TODO create an event listener for when other scenes have finished loading
public:
    WorldTransition(Game& game, const std::string& name);
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    float timer = 0.0f;
    float animDuration = 1.0f;
    TransitionState state = TransitionState::CLOSING;
    float closingStartTime = 0.0f;
    Vector2 playerScreenPosition = { 0.0f, 0.0f };
};