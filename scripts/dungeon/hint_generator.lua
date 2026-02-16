-- scripts/dungeon/hint_generator.lua
local HintGenerator = {}

-- mission-critical items eligible to be hinted at
local hint_items = { "weapon_sword", "boss_key" }

-- npcs that can deliver hints (keys as in npcs.json)
local hint_npcs = { "randomNPC1" }

-- item names
-- TODO use names from items data
local item_display_names = {
    weapon_sword = "Sword",
    boss_key     = "Boss Key",
    key          = "a key"
}

local hint_template_key    = "npcHint1"
local generic_dialogue_key = "npcHintGeneric1"

-- capitalize each word and replace underscores with spaces
local function prettify_name(str)
    return (str:gsub("_", " "):gsub("(%a)([%w]*)", function(first, rest)
        return first:upper() .. rest
    end))
end

-- find which hint_items are actually placed in the graph
local function find_placed_items(graph)
    local placed = {}
    local found_set = {}

    for _, item_name in ipairs(hint_items) do
        for _, node in pairs(graph.nodes) do
            if node.value == item_name and not found_set[item_name] then
                table.insert(placed, item_name)
                found_set[item_name] = true
                break
            end
        end
    end

    return placed
end

local function load_json(path)
    local file = io.open(path, "r")
    if not file then
        error("HintGenerator: could not open file: " .. path)
    end
    local content = file:read("*all")
    file:close()
    return json.decode(content)
end

local function save_json(path, data)
    local file = io.open(path, "w")
    if not file then
        error("HintGenerator: could not write file: " .. path)
    end
    file:write(json.encode(data, 2))
    file:close()
end

local function patch_npc_dialogue(path, npc_key, dialogue_key)
    -- uses text replacement instead of json loading since lua would break empty arrays in json...
    local file = io.open(path, "r")
    if not file then
        error("HintGenerator: could not open file: " .. path)
    end
    local content = file:read("*all")
    file:close()

    -- find the position of the npc entry
    local npc_pos = content:find('"' .. npc_key .. '"', 1, true)
    if not npc_pos then
        error("HintGenerator: NPC key not found in file: " .. npc_key)
    end

    -- find "dialogue": "..." after the npc position
    local dial_start, dial_end = content:find('"dialogue"%s*:%s*"[^"]*"', npc_pos)
    if not dial_start then
        error("HintGenerator: could not patch dialogue for " .. npc_key .. " — dialogue key not found in entry")
    end

    local new_content = content:sub(1, dial_start - 1)
        .. '"dialogue": "' .. dialogue_key .. '"'
        .. content:sub(dial_end + 1)

    local out = io.open(path, "w")
    out:write(new_content)
    out:close()
end

local function resolve_hint_text(template, item_name, dungeon_key, dungeon_display_name)
    local display_item = item_display_names[item_name] or prettify_name(item_name)
    local result = template
    result = result:gsub("%[HINT:item%]", display_item)
    result = result:gsub("%[HINT:loc%]",  dungeon_display_name)
    result = result:gsub("%[HINT:dir%]",  "???")
    return result
end

function HintGenerator.generate(graph, dungeon_key, dungeon_display_name, texts_path, npcs_path)
    texts_path = texts_path or "resources/texts.json"
    npcs_path  = npcs_path  or "resources/npcs.json"

    print("\nStep 5b: Generating NPC hints...")

    local texts_data = load_json(texts_path)

    if not texts_data[hint_template_key] then
        error("HintGenerator: missing template key in texts.json: " .. hint_template_key)
    end
    if not texts_data[generic_dialogue_key] then
        error("HintGenerator: missing generic dialogue key in texts.json: " .. generic_dialogue_key)
    end

    local template     = texts_data[hint_template_key][1]
    local placed_items = find_placed_items(graph)
    print(string.format("  Found %d hintable item(s), %d hint NPC(s) available", #placed_items, #hint_npcs))

    for i, npc_key in ipairs(hint_npcs) do
        local item_name    = placed_items[i]
        local dialogue_key

        if item_name then
            dialogue_key = string.format("hint_%s_%s", dungeon_key, npc_key)
            texts_data[dialogue_key] = { resolve_hint_text(template, item_name, dungeon_key, dungeon_display_name) }
            print(string.format("  %s -> '%s' (%s)", npc_key, dialogue_key, item_name))
        else
            dialogue_key = generic_dialogue_key
            print(string.format("  %s -> generic fallback", npc_key))
        end

        patch_npc_dialogue(npcs_path, npc_key, dialogue_key)
    end

    save_json(texts_path, texts_data)

    print("  Hint generation complete.")
end

return HintGenerator