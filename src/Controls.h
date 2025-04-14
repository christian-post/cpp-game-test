#pragma once
#include <cstdint>
#include <iostream>

enum Controls : uint32_t {
    // creates Bitmasks for each of the buttons
    CONTROL_NONE = 0,
    CONTROL_UP = 1 << 0,
    CONTROL_DOWN = 1 << 1,
    CONTROL_LEFT = 1 << 2,
    CONTROL_RIGHT = 1 << 3,
    CONTROL_ACTION1 = 1 << 4,
    CONTROL_ACTION2 = 1 << 5,
    CONTROL_ACTION3 = 1 << 6,
    CONTROL_ACTION4 = 1 << 7,
    CONTROL_CONFIRM = 1 << 8,
    CONTROL_CANCEL = 1 << 9,
    CONTROL_DEBUG = 1 << 10
};

// function that takes one of the other functions below and checks for each of the Controls
using KeyCheckFunc = bool (*)(int);
using GamepadCheckFunc = bool (*)(int, int);

uint32_t GetControls(KeyCheckFunc keyFunc, GamepadCheckFunc gamepadFunc);

uint32_t GetControlsDown();  // if a key or button is being held down
uint32_t GetControlsPressed();  // if that key or button is pressed once
uint32_t GetControlsUp(); // you get the idea

bool AnyKeyPressed(uint32_t controls);