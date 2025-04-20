#pragma once
#include <queue>
#include <vector>
#include "Commands.h"

struct QueuedCommand {
    Command* cmd;
    bool blocking;
};

class CutsceneManager {
    std::queue<QueuedCommand> commands;
    std::vector<Command*> nonBlocking;
    bool active = false;

public:
    void queueCommand(Command* cmd, bool blocking = true);
    void update(float dt);
    void draw();
    bool isActive() const { return active; }

    std::string currentCommandName() const {
        if (!commands.empty()) {
            return commands.front().cmd->name;
        }
        return "empty";
    }
};
