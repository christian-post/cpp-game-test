local DirectionUtils = require("dungeon.direction_utils")

local HintGenerator = {}

-- mission-critical items that can be hinted at
local hint_items = { "weapon_sword" }

-- npcs that can deliver hints (keys as in npcs.json)
local hint_npcs = { 
    { key = "randomNPC1", world = "interior", room = {0, 0}, level = 0 },
    { key = "randomNPC2", world = "overworld", room = {1, 1}, level = 0 }
}

-- locations on the overworld that get used in conversations
-- TODO get the display_name from dungeons.json?
local famous_locations = {
    { room = {0, 0}, display_name = "Clifftop Fortress" },
    { room = {1, 1}, display_name = "the village" }
}

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
local function find_placed_items(graph, layout, dungeon_key)
    local placed = {}
    local found_set = {}

    for _, item_name in ipairs(hint_items) do
        for node_name, node in pairs(graph.nodes) do
            if node.value == item_name and not found_set[item_name] then
                -- look up position from layout
                local pos = layout.positions[node_name]
                if not pos then
                    error(string.format("HintGenerator: node '%s' not found in layout", node_name))
                end
                
                table.insert(placed, {
                    name = item_name,
                    world = dungeon_key,
                    room = {pos.row, pos.col},
                    level = pos.level
                })
                found_set[item_name] = true
                break
            end
        end
    end

    return placed
end

-- TODO these should be in a utils script
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

local function resolve_hint_text(template, item, npc, dungeon_display_name, dungeons_data)
    local display_item = item_display_names[item.name] or prettify_name(item.name)
    
    -- resolve both locations to overworld coordinates
    local npc_overworld = DirectionUtils.resolve_to_overworld_room(npc.world, npc.room, npc.level, dungeons_data)
    local item_overworld = DirectionUtils.resolve_to_overworld_room(item.world, item.room, item.level, dungeons_data)
    
    -- get direction string
    local direction_str = DirectionUtils.get_direction_string(npc_overworld, item_overworld, famous_locations)
    
    local result = template
    result = result:gsub("%[HINT:item%]", display_item)
    result = result:gsub("%[HINT:loc%]",  dungeon_display_name)
    result = result:gsub("%[HINT:dir%]",  direction_str)
    return result
end

function HintGenerator.generate(graph, layout, dungeon_key, dungeon_display_name, texts_path, npcs_path)
    -- TODO file path definitions consistency
    texts_path = texts_path or "resources/texts.json"
    npcs_path  = npcs_path  or "resources/npcs.json"

    print("\nStep 8: Generating NPC hints...")

    local texts_data = load_json(texts_path)
    local dungeons_data = load_json("resources/dungeons.json")

    if not texts_data[hint_template_key] then
        error("HintGenerator: missing template key in texts.json: " .. hint_template_key)
    end
    if not texts_data[generic_dialogue_key] then
        error("HintGenerator: missing generic dialogue key in texts.json: " .. generic_dialogue_key)
    end

    local template     = texts_data[hint_template_key][1]
    local placed_items = find_placed_items(graph, layout, dungeon_key)

    for i, npc in ipairs(hint_npcs) do
        local item = placed_items[i]
        local dialogue_key

        if item then
            dialogue_key = string.format("hint_%s_%s", dungeon_key, npc.key)
            texts_data[dialogue_key] = { resolve_hint_text(template, item, npc, dungeon_display_name, dungeons_data) }
            print(string.format("  %s -> '%s' (%s)", npc.key, dialogue_key, item.name))
        else
            dialogue_key = generic_dialogue_key
            print(string.format("  %s -> generic fallback", npc.key))
        end

        patch_npc_dialogue(npcs_path, npc.key, dialogue_key)
    end

    save_json(texts_path, texts_data)

    print("  Hint generation complete.")
end

return HintGenerator