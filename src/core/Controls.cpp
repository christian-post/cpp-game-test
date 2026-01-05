#include "Controls.h"
#include "raylib.h"

static bool lastWasGamepad = false;
static std::unordered_map<uint32_t, std::vector<int>> controlKeyMap;
static std::unordered_map<uint32_t, std::vector<int>> controlGamepadMap;

bool WasGamepadUsedLast()
{
    return lastWasGamepad;
}

static bool keyDown(int key)
{
    bool down = IsKeyDown(key);
    if (down) 
        lastWasGamepad = false;
    return down;
}

static bool keyPressed(int key)
{
    bool pressed = IsKeyPressed(key);
    if (pressed)
        lastWasGamepad = false;
    return pressed;
}

static bool keyReleased(int key)
{
    bool released = IsKeyReleased(key);
    if (released)
        lastWasGamepad = false;
    return released;
}

static bool gamepadDown(int gamepad, int button)
{
    bool down = IsGamepadButtonDown(gamepad, button);
    if (down)
        lastWasGamepad = true;
    return down;
}

static bool gamepadPressed(int gamepad, int button)
{
    bool pressed = IsGamepadButtonPressed(gamepad, button);
    if (pressed)
        lastWasGamepad = true;
    return pressed;
}

static bool gamepadReleased(int gamepad, int button)
{
    bool released = IsGamepadButtonReleased(gamepad, button);
    if (released)
        lastWasGamepad = true;
    return released;
}

void SetDefaultKeyBindings()
{
    controlKeyMap.clear();
    controlKeyMap[CONTROL_UP] = { KEY_W, KEY_UP };
    controlKeyMap[CONTROL_DOWN] = { KEY_S, KEY_DOWN };
    controlKeyMap[CONTROL_LEFT] = { KEY_A, KEY_LEFT };
    controlKeyMap[CONTROL_RIGHT] = { KEY_D, KEY_RIGHT };
    controlKeyMap[CONTROL_ACTION1] = { KEY_O };
    controlKeyMap[CONTROL_ACTION2] = { KEY_P };
    controlKeyMap[CONTROL_ACTION3] = { KEY_K };
    controlKeyMap[CONTROL_ACTION4] = { KEY_L };
    controlKeyMap[CONTROL_ACTIONL] = { KEY_N };
    controlKeyMap[CONTROL_ACTIONR] = { KEY_M };
    controlKeyMap[CONTROL_CONFIRM] = { KEY_ENTER };
    controlKeyMap[CONTROL_CANCEL] = { KEY_BACKSPACE };
    controlKeyMap[CONTROL_DEBUG] = { KEY_F1 };
    controlKeyMap[CONTROL_DEBUG2] = { KEY_F2 };
    controlKeyMap[CONTROL_DEBUG_K1] = { KEY_KP_1 };
    controlKeyMap[CONTROL_DEBUG_K2] = { KEY_KP_2 };
    controlKeyMap[CONTROL_DEBUG_K3] = { KEY_KP_3 };
}

void SetDefaultGamepadBindings()
{
    controlGamepadMap.clear();
    controlGamepadMap[CONTROL_UP] = { GAMEPAD_BUTTON_LEFT_FACE_UP };
    controlGamepadMap[CONTROL_DOWN] = { GAMEPAD_BUTTON_LEFT_FACE_DOWN };
    controlGamepadMap[CONTROL_LEFT] = { GAMEPAD_BUTTON_LEFT_FACE_LEFT };
    controlGamepadMap[CONTROL_RIGHT] = { GAMEPAD_BUTTON_LEFT_FACE_RIGHT };
    controlGamepadMap[CONTROL_ACTION1] = { GAMEPAD_BUTTON_RIGHT_FACE_DOWN };
    controlGamepadMap[CONTROL_ACTION2] = { GAMEPAD_BUTTON_RIGHT_FACE_LEFT };
    controlGamepadMap[CONTROL_ACTION3] = { GAMEPAD_BUTTON_RIGHT_FACE_UP };
    controlGamepadMap[CONTROL_ACTION4] = { GAMEPAD_BUTTON_RIGHT_FACE_RIGHT };
    controlGamepadMap[CONTROL_ACTIONL] = { GAMEPAD_BUTTON_LEFT_TRIGGER_1 };
    controlGamepadMap[CONTROL_ACTIONR] = { GAMEPAD_BUTTON_RIGHT_TRIGGER_1 };
    controlGamepadMap[CONTROL_CONFIRM] = { GAMEPAD_BUTTON_MIDDLE_RIGHT };
    controlGamepadMap[CONTROL_CANCEL] = { GAMEPAD_BUTTON_MIDDLE_LEFT };
}

void InitializeKeyBindings(const nlohmann::json& keyBindings)
{
    controlKeyMap.clear();

    if (keyBindings.is_null() || !keyBindings.is_array())
    {
        TraceLog(LOG_WARNING, "Invalid keyBindings JSON, using defaults");
        SetDefaultKeyBindings();
        return;
    }

    for (size_t i = 0; i < keyBindings.size(); ++i)
    {
        const auto& keysJson = keyBindings[i];

        if (!keysJson.is_array())
        {
            TraceLog(LOG_WARNING, "keyBindings[%zu] is not an array", i);
            continue;
        }

        std::vector<int> keycodes;
        for (const auto& keyJson : keysJson)
        {
            if (!keyJson.is_number_integer())
            {
                TraceLog(LOG_WARNING, "Non-integer keycode in keyBindings[%zu]", i);
                continue;
            }
            keycodes.push_back(keyJson.get<int>());
        }

        if (!keycodes.empty())
        {
            uint32_t control = 1u << i; // convert array index to bit flag
            controlKeyMap[control] = keycodes;
        }
    }

    if (controlKeyMap.empty())
    {
        TraceLog(LOG_WARNING, "No valid key bindings found, using defaults");
        SetDefaultKeyBindings();
    }
}

void InitializeGamepadBindings(const nlohmann::json& gamepadBindings)
{
    controlGamepadMap.clear();

    if (gamepadBindings.is_null() || !gamepadBindings.is_array())
    {
        TraceLog(LOG_WARNING, "Invalid gamepadBindings JSON, using defaults");
        SetDefaultGamepadBindings();
        return;
    }

    for (size_t i = 0; i < gamepadBindings.size(); ++i)
    {
        const auto& buttonsJson = gamepadBindings[i];

        if (!buttonsJson.is_array())
        {
            TraceLog(LOG_WARNING, "gamepadBindings[%zu] is not an array", i);
            continue;
        }

        std::vector<int> buttons;
        for (const auto& buttonJson : buttonsJson)
        {
            if (!buttonJson.is_number_integer())
            {
                TraceLog(LOG_WARNING, "Non-integer button in gamepadBindings[%zu]", i);
                continue;
            }
            buttons.push_back(buttonJson.get<int>());
        }

        if (!buttons.empty())
        {
            uint32_t control = 1u << i;
            controlGamepadMap[control] = buttons;
        }
    }

    if (controlGamepadMap.empty())
    {
        TraceLog(LOG_WARNING, "No valid gamepad bindings found, using defaults");
        SetDefaultGamepadBindings();
    }
}

uint32_t GetControls(KeyCheckFunc keyFunc, GamepadCheckFunc gamepadFunc)
{
    uint32_t controls = CONTROL_NONE;

    // check keyboard bindings
    for (const auto& [control, keycodes] : controlKeyMap)
    {
        for (int keycode : keycodes)
        {
            if (keyFunc(keycode))
            {
                controls |= control;
                break;
            }
        }
    }

    // check gamepad bindings
    if (IsGamepadAvailable(0))
    {
        for (const auto& [control, buttons] : controlGamepadMap)
        {
            for (int button : buttons)
            {
                if (gamepadFunc(0, button))
                {
                    controls |= control;
                    break;
                }
            }
        }
    }

    return controls;
}

uint32_t GetControlsDown()
{
    return GetControls(keyDown, gamepadDown);
}

uint32_t GetControlsPressed()
{
    return GetControls(keyPressed, gamepadPressed);
}

uint32_t GetControlsUp()
{
    return GetControls(keyReleased, gamepadReleased);
}

bool AnyKeyPressed(uint32_t controls)
{
    return controls != CONTROL_NONE;
}
