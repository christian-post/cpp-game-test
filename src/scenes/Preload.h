#pragma once

#include "Scene.h"
#include <iostream>
#include <string>
#include <functional>
#include <queue>

class Preload : public Scene {
public:
    Preload(Game& game, const std::string& name) : Scene(game, name) {}
    void startup() override;
    void update(float deltaTime) override;
    void draw() override;
    void end() override;

private:
    std::queue<std::pair<std::string, std::function<void()>>> loadQueue;
    std::string currentMessage = "Loading...";
    size_t totalLoadSteps = 0;
};