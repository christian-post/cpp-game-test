#include "Controls.h"
#include "raylib.h"


uint32_t GetControls(KeyCheckFunc keyFunc, GamepadCheckFunc gamepadFunc) {
    uint32_t controls = CONTROL_NONE;

    // Keyboard input
    // TODO get key mapping from settings.json
    if (keyFunc(KEY_UP) || keyFunc(KEY_W)) controls |= CONTROL_UP;
    if (keyFunc(KEY_DOWN) || keyFunc(KEY_S)) controls |= CONTROL_DOWN;
    if (keyFunc(KEY_LEFT) || keyFunc(KEY_A)) controls |= CONTROL_LEFT;
    if (keyFunc(KEY_RIGHT) || keyFunc(KEY_D)) controls |= CONTROL_RIGHT;
    if (keyFunc(KEY_O)) controls |= CONTROL_ACTION1;
    if (keyFunc(KEY_P)) controls |= CONTROL_ACTION2;
    if (keyFunc(KEY_K)) controls |= CONTROL_ACTION3;
    if (keyFunc(KEY_L)) controls |= CONTROL_ACTION4;
    if (keyFunc(KEY_ENTER)) controls |= CONTROL_CONFIRM;
    if (keyFunc(KEY_BACKSPACE)) controls |= CONTROL_CANCEL;
    if (keyFunc(KEY_F1)) controls |= CONTROL_DEBUG;
    if (keyFunc(KEY_KP_1)) controls |= CONTROL_DEBUG_K1;
    if (keyFunc(KEY_KP_1)) controls |= CONTROL_DEBUG_K2;

    // Gamepad input
    if (IsGamepadAvailable(0)) {
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) controls |= CONTROL_UP;
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) controls |= CONTROL_DOWN;
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) controls |= CONTROL_LEFT;
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) controls |= CONTROL_RIGHT;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) controls |= CONTROL_ACTION1;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) controls |= CONTROL_ACTION2;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) controls |= CONTROL_ACTION3;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) controls |= CONTROL_ACTION4;
        if (gamepadFunc(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) controls |= CONTROL_CONFIRM;
        if (gamepadFunc(0, GAMEPAD_BUTTON_MIDDLE_LEFT)) controls |= CONTROL_CANCEL;
    }

    return controls;
}

// Wrapper functions for readability
uint32_t GetControlsDown() {
    return GetControls(IsKeyDown, IsGamepadButtonDown);
}

uint32_t GetControlsPressed() {
    return GetControls(IsKeyPressed, IsGamepadButtonPressed);
}

uint32_t GetControlsUp() {
    return GetControls(IsKeyUp, IsGamepadButtonReleased);
}

bool AnyKeyPressed(uint32_t controls) {
    // just checks if controls is 0
    return controls != CONTROL_NONE;
}