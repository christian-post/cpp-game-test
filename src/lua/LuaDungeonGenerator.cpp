#include "LuaDungeonGenerator.h"
#include "Game.h"


LuaDungeonGenerator::LuaDungeonGenerator(Game& game)
    : game(game)
{
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::package, sol::lib::table, sol::lib::os, sol::lib::io);
    lua["package"]["path"] = lua["package"]["path"].get<std::string>() + ";scripts/?.lua";

    setupBindings();
}

void LuaDungeonGenerator::setSeed(int seed)
{
    //lua["math"]["randomseed"](seed); TODO probably not needed anymore
    dungeonRng.seed(seed);
    lua["dungeon_seed"] = seed;
}

void LuaDungeonGenerator::setupBindings()
{
    // expose json encoding to lua
    lua["json"] = lua.create_table();

    lua["json"]["encode"] = [this](sol::table tbl, sol::optional<int> indent) -> std::string
    {
        nlohmann::json j = solToJson(tbl);
        int indent_spaces = indent.value_or(2);
        return j.dump(indent_spaces);
    };

    lua["json"]["decode"] = [this](const std::string& json_str) -> sol::object
        {
            try
            {
                nlohmann::json j = nlohmann::json::parse(json_str);
                return jsonToSol(j);  // Convert JSON → Lua
            }
            catch (const nlohmann::json::exception& e)
            {
                TraceLog(LOG_ERROR, "JSON decode error: %s", e.what());
                return sol::nil;
            }
        };

    // keeps the raylib window busy while the dungeon script is running
    lua["yield_to_engine"] = [this](sol::optional<std::string> message) {
        if (message)
            // include a message that the loading scene can use to display information
            this->game.eventManager.pushEvent(DUNGEON_GENERATION_TICK, *message);
        else
            this->game.eventManager.pushEvent(DUNGEON_GENERATION_TICK, std::any{});
        this->game.processFrame();
        return !WindowShouldClose();
        };

    lua["update_progress"] = [this](sol::optional<std::string> message) {
        if (message)
            // include a message that the loading scene can use to display information
            this->game.eventManager.pushEvent(DUNGEON_GENERATION_PROGRESS, *message);
        else
            this->game.eventManager.pushEvent(DUNGEON_GENERATION_PROGRESS, std::any{});
        };

    lua["dungeon_generation_start"] = [this]() {
        this->game.eventManager.pushEvent(DUNGEON_GENERATION_START);
        };

    lua["dungeon_generation_complete"] = [this](sol::optional<bool> success) {
        bool result = success.value_or(true); // expect "true" as default
        this->game.eventManager.pushEvent(DUNGEON_GENERATION_COMPLETE, result);
        };

    // dungeon-specific RNG, won't be affected by game frame processing
    lua["dungeon_random"] = [this](sol::optional<int> m, sol::optional<int> n) -> double
        {
            std::uniform_real_distribution<double> dist(0.0, 1.0);
            double r = dist(dungeonRng);

            if (m && n)
            {
                // dungeon_random(m, n) -> integer in range [m, n]
                return std::floor(r * (*n - *m + 1)) + *m;
            }
            else if (m)
            {
                // dungeon_random(m) -> integer in range [1, m]
                return std::floor(r * *m) + 1;
            }
            else
            {
                // dungeon_random() -> float in range [0, 1)
                return r;
            }
        };

    // filesystem operations
    lua["filesystem"] = lua.create_table();

    lua["filesystem"]["create_directory"] = [](const std::string& path) {
        return std::filesystem::create_directories(path);
        };

    lua["filesystem"]["clear_directory"] = [](const std::string& path) {
        if (!std::filesystem::exists(path))
            return;

        for (const auto& entry : std::filesystem::directory_iterator(path))
        {
            std::filesystem::remove_all(entry.path());
        }
        };
}

nlohmann::json LuaDungeonGenerator::solToJson(const sol::object& obj)
{
    if (obj.is<sol::table>())
    {
        sol::table tbl = obj.as<sol::table>();

        // check if it's an array (lua arrays: sequential keys starting at 1)
        bool is_array = true;
        size_t max_index = 0;

        for (const auto& pair : tbl)
        {
            sol::object key = pair.first;
            if (key.is<int>())
            {
                int idx = key.as<int>();
                if (idx > 0 && (size_t)idx > max_index)
                {
                    max_index = idx;
                }
            }
            else
            {
                is_array = false;
                break;
            }
        }

        // verify it's truly sequential
        if (is_array && max_index > 0)
        {
            for (size_t i = 1; i <= max_index; i++)
            {
                if (!tbl[i].valid())
                {
                    is_array = false;
                    break;
                }
            }
        }

        if (is_array && max_index > 0)
        {
            // convert to json array
            nlohmann::json arr = nlohmann::json::array();
            for (size_t i = 1; i <= max_index; i++)
            {
                arr.push_back(solToJson(tbl[i]));
            }
            return arr;
        }
        else
        {
            // convert to json object
            nlohmann::json obj_json = nlohmann::json::object();
            for (const auto& pair : tbl)
            {
                sol::object key = pair.first;
                sol::object value = pair.second;

                std::string key_str;
                if (key.is<std::string>())
                {
                    key_str = key.as<std::string>();
                }
                else if (key.is<int>())
                {
                    key_str = std::to_string(key.as<int>());
                }
                else
                {
                    continue; // skip other key types
                }

                obj_json[key_str] = solToJson(value);
            }
            return obj_json;
        }
    }
    else if (obj.is<std::string>())
    {
        return obj.as<std::string>();
    }
    else if (obj.is<int>())
    {
        return obj.as<int>();
    }
    else if (obj.is<double>())
    {
        return obj.as<double>();
    }
    else if (obj.is<bool>())
    {
        return obj.as<bool>();
    }
    else if (obj.is<sol::nil_t>())
    {
        return nullptr;
    }

    return nullptr;
}

sol::object LuaDungeonGenerator::jsonToSol(const nlohmann::json& j)
{
    if (j.is_null())
    {
        return sol::nil;
    }
    else if (j.is_boolean())
    {
        return sol::make_object(lua, j.get<bool>());
    }
    else if (j.is_number_integer())
    {
        return sol::make_object(lua, j.get<int>());
    }
    else if (j.is_number_float())
    {
        return sol::make_object(lua, j.get<double>());
    }
    else if (j.is_string())
    {
        return sol::make_object(lua, j.get<std::string>());
    }
    else if (j.is_array())
    {
        sol::table arr = lua.create_table();
        int index = 1;
        for (const auto& item : j)
        {
            arr[index++] = jsonToSol(item);
        }
        return arr;
    }
    else if (j.is_object())
    {
        sol::table obj = lua.create_table();
        for (auto it = j.begin(); it != j.end(); ++it)
        {
            obj[it.key()] = jsonToSol(it.value());
        }
        return obj;
    }

    return sol::nil;
}
