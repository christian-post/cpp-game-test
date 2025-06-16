#include "Controls.h"
#include "raylib.h"

static bool lastWasGamepad = false;

bool WasGamepadUsedLast() {
    return lastWasGamepad;
}

bool keyDown(int key) {
    bool down = IsKeyDown(key);
    if (down) lastWasGamepad = false;
    return down;
}

bool keyPressed(int key) {
    bool pressed = IsKeyPressed(key);
    if (pressed) lastWasGamepad = false;
    return pressed;
}

bool keyReleased(int key) {
    bool released = IsKeyReleased(key);
    if (released) lastWasGamepad = false;
    return released;
}

bool gamepadDown(int gamepad, int button) {
    bool down = IsGamepadButtonDown(gamepad, button);
    if (down) lastWasGamepad = true;
    return down;
}

bool gamepadPressed(int gamepad, int button) {
    bool pressed = IsGamepadButtonPressed(gamepad, button);
    if (pressed) lastWasGamepad = true;
    return pressed;
}

bool gamepadReleased(int gamepad, int button) {
    bool released = IsGamepadButtonReleased(gamepad, button);
    if (released) lastWasGamepad = true;
    return released;
}


uint32_t GetControls(KeyCheckFunc keyFunc, GamepadCheckFunc gamepadFunc) {
    uint32_t controls = CONTROL_NONE;

    if (keyFunc(KEY_UP) || keyFunc(KEY_W)) controls |= CONTROL_UP;
    if (keyFunc(KEY_DOWN) || keyFunc(KEY_S)) controls |= CONTROL_DOWN;
    if (keyFunc(KEY_LEFT) || keyFunc(KEY_A)) controls |= CONTROL_LEFT;
    if (keyFunc(KEY_RIGHT) || keyFunc(KEY_D)) controls |= CONTROL_RIGHT;
    if (keyFunc(KEY_O)) controls |= CONTROL_ACTION1;
    if (keyFunc(KEY_P)) controls |= CONTROL_ACTION2;
    if (keyFunc(KEY_K)) controls |= CONTROL_ACTION3;
    if (keyFunc(KEY_L)) controls |= CONTROL_ACTION4;
    if (keyFunc(KEY_M)) controls |= CONTROL_ACTIONR;
    if (keyFunc(KEY_N)) controls |= CONTROL_ACTIONL;
    if (keyFunc(KEY_ENTER)) controls |= CONTROL_CONFIRM;
    if (keyFunc(KEY_BACKSPACE)) controls |= CONTROL_CANCEL;
    if (keyFunc(KEY_F1)) controls |= CONTROL_DEBUG;
    if (keyFunc(KEY_KP_1)) controls |= CONTROL_DEBUG_K1;
    if (keyFunc(KEY_KP_2)) controls |= CONTROL_DEBUG_K2;
    if (keyFunc(KEY_KP_3)) controls |= CONTROL_DEBUG_K3;

    if (IsGamepadAvailable(0)) {
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_FACE_UP)) controls |= CONTROL_UP;
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_FACE_DOWN)) controls |= CONTROL_DOWN;
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_FACE_LEFT)) controls |= CONTROL_LEFT;
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_FACE_RIGHT)) controls |= CONTROL_RIGHT;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_FACE_DOWN)) controls |= CONTROL_ACTION1;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_FACE_LEFT)) controls |= CONTROL_ACTION2;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_FACE_UP)) controls |= CONTROL_ACTION3;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_FACE_RIGHT)) controls |= CONTROL_ACTION4;
        if (gamepadFunc(0, GAMEPAD_BUTTON_MIDDLE_RIGHT)) controls |= CONTROL_CONFIRM;
        if (gamepadFunc(0, GAMEPAD_BUTTON_MIDDLE_LEFT)) controls |= CONTROL_CANCEL;
        if (gamepadFunc(0, GAMEPAD_BUTTON_LEFT_TRIGGER_1)) controls |= CONTROL_ACTIONL;
        if (gamepadFunc(0, GAMEPAD_BUTTON_RIGHT_TRIGGER_1)) controls |= CONTROL_ACTIONR;
    }

    return controls;
}

uint32_t GetControlsDown() {
    return GetControls(keyDown, gamepadDown);
}

uint32_t GetControlsPressed() {
    return GetControls(keyPressed, gamepadPressed);
}

uint32_t GetControlsUp() {
    return GetControls(keyReleased, gamepadReleased);
}

bool AnyKeyPressed(uint32_t controls) {
    return controls != CONTROL_NONE;
}
