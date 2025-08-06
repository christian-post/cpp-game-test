#pragma once
#include <queue>
#include <vector>
#include <memory>
#include <string>
#include "Commands.h"


struct QueuedCommand {
    Command* cmd;
    bool blocking;
};

class CutsceneManager {
    std::queue<QueuedCommand> commands;
    std::vector<Command*> nonBlocking;
    bool active = false;
    bool controlsCamera = false;

public:
    void queueCommand(Command* cmd, bool blocking = true);
    void update(float deltaTime);
    void draw();
    bool isActive() const { return active; }
    bool hasCameraControl() const { return controlsCamera; }
    void setCameraControl(bool set) { controlsCamera = set; }

    std::string currentCommandName() const {
        if (!commands.empty()) {
            return commands.front().cmd->name;
        }
        return "empty";
    }
};
