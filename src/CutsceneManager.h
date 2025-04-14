// CutsceneManager.h
#pragma once
#include <queue>
#include "Commands.h"

class CutsceneManager {
    std::queue<Command*> commands;
    bool active = false;

public:
    void queueCommand(Command* cmd);
    void update(float dt);
    void draw();
    bool isActive() const { return active; }
};
