#pragma once
#include <functional>
#include <raylib.h>
#include <raymath.h>

class WindowEventHandler {
public:
    std::function<void(int, int)> onResize;
    std::function<void(Vector2)> onReposition;

    void update() {
        if (IsWindowResized())
            if (onResize)
                onResize(GetScreenWidth(), GetScreenHeight());

        Vector2 pos = GetWindowPosition();
        if (pos.x != lastPos.x || pos.y != lastPos.y)
            if (onReposition)
                onReposition(pos);
        lastPos = pos;
    }

private:
    Vector2 lastPos = { 0, 0 };
};