#pragma once
#include "Commands.h"
#include <queue>
#include <vector>
#include <memory>
#include <string>


struct QueuedCommand
{
    Command* cmd;
    bool blocking;
};

class CutsceneManager
{
public:
    void queueCommand(Command* cmd, bool blocking = true);
    void update(float deltaTime);
    void draw();
    bool isActive() const { return active; }
    bool hasCameraControl() const { return controlsCamera; }
    void setCameraControl(bool set) { controlsCamera = set; }

    std::string currentCommandName() const
    {
        if (!commands.empty())
            return commands.front().cmd->name;
        return "empty";
    }

    // internal state that is manipulated by the commands
    float letterBoxBarHeight = 0.0f; // draw black bars when in letterbox mode

private:
    std::queue<QueuedCommand> commands;
    std::vector<Command*> nonBlocking;
    bool active = false;
    bool controlsCamera = false;
};
