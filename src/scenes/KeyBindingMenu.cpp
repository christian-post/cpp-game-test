#include "KeyBindingMenu.h"
#include "Controls.h"

KeyBindingMenu::KeyBindingMenu(Game& game, const std::string& name)
    : Scene(game, name), menu(MenuSelect(game))
{
    menu.setFontSize(6);
    menu.setYMargin(8);
}

void KeyBindingMenu::startup()
{
    std::vector<ControlBinding> bindings = {
        // keyboard bindings
        {"Move Up - Key (1)", 0, 0, false},
        {"Move Up - Key (2)", 0, 1, false},
        {"Move Down - Key (1)", 1, 0, false},
        {"Move Down - Key (2)", 1, 1, false},
        {"Move Left - Key (1)", 2, 0, false},
        {"Move Left - Key (2)", 2, 1, false},
        {"Move Right - Key (1)", 3, 0, false},
        {"Move Right - Key (2)", 3, 1, false},
        {"Action 1 - Key", 4, 0, false},
        {"Action 2 - Key", 5, 0, false},
        {"Action 3 - Key", 6, 0, false},
        {"Action 4 - Key", 7, 0, false},
        {"Action L - Key", 8, 0, false},
        {"Action R - Key", 9, 0, false},
        {"Confirm - Key", 10, 0, false},
        {"Cancel - Key", 11, 0, false},

        // gamepad bindings
        {"Move Up - Gamepad", 0, 0, true},
        {"Move Down - Gamepad", 1, 0, true},
        {"Move Left - Gamepad", 2, 0, true},
        {"Move Right - Gamepad", 3, 0, true},
        {"Action 1 - Gamepad", 4, 0, true},
        {"Action 2 - Gamepad", 5, 0, true},
        {"Action 3 - Gamepad", 6, 0, true},
        {"Action 4 - Gamepad", 7, 0, true},
        {"Action L - Gamepad", 8, 0, true},
        {"Action R - Gamepad", 9, 0, true},
        {"Confirm - Gamepad", 10, 0, true},
        {"Cancel - Gamepad", 11, 0, true}
    };

    auto& settings = game.loader.getSettings();
    nlohmann::json keyBindingsArray;
    nlohmann::json gamepadBindingsArray;

    if (settings.contains("keyBindings") && settings["keyBindings"].is_array())
        keyBindingsArray = settings["keyBindings"];
    if (settings.contains("gamepadBindings") && settings["gamepadBindings"].is_array())
        gamepadBindingsArray = settings["gamepadBindings"];

    for (const auto& binding : bindings)
    {
        MenuItem item;
        item.displayName = binding.name;
        item.type = MenuItemType::KeyBind;

        // load current binding
        auto& bindingsArray = binding.isGamepad ? gamepadBindingsArray : keyBindingsArray;

        if (binding.arrayIndex < bindingsArray.size() &&
            bindingsArray[binding.arrayIndex].is_array() &&
            binding.keyIndex < bindingsArray[binding.arrayIndex].size())
        {
            item.boundKey = bindingsArray[binding.arrayIndex][binding.keyIndex].get<int>();
        }

        item.keyBindCallback = [this, binding](int keycode) {
            // validate input type matches binding type
            bool isGamepadInput = (keycode < 32);
            if (isGamepadInput != binding.isGamepad)
            {
                TraceLog(LOG_WARNING, "Wrong input type: expected %s",
                    binding.isGamepad ? "gamepad" : "keyboard");
                return;
            }

            saveBinding(binding.arrayIndex, binding.keyIndex, keycode, binding.isGamepad);
            };

        menu.addItem(item);
    }

    menu.addItem({ "Back", MenuItemType::Action, [&]() {
        game.stopScene(getName());
        game.saveSettings();
    } });
}

void KeyBindingMenu::saveBinding(size_t arrayIndex, size_t keyIndex, int keycode, bool isGamepad)
{
    auto& settings = game.loader.getSettings();

    std::string settingKey = isGamepad ? "gamepadBindings" : "keyBindings";

    // ensure bindings array exists
    if (!settings.contains(settingKey) || !settings[settingKey].is_array())
        settings[settingKey] = nlohmann::json::array();

    auto& bindingsArray = settings[settingKey];

    // ensure array is large enough
    while (bindingsArray.size() <= arrayIndex)
        bindingsArray.push_back(nlohmann::json::array());

    // ensure sub-array is large enough
    while (bindingsArray[arrayIndex].size() <= keyIndex)
        bindingsArray[arrayIndex].push_back(0);

    // save the new keycode
    bindingsArray[arrayIndex][keyIndex] = keycode;

    game.writeSetting(settingKey, bindingsArray);

    // reinitialize controls with new bindings
    if (isGamepad)
        InitializeGamepadBindings(bindingsArray);
    else
        InitializeKeyBindings(bindingsArray);
}

void KeyBindingMenu::update(float deltaTime)
{
    menu.update();
}

void KeyBindingMenu::draw()
{
    menu.draw();
}