local TilemapModifier = {}

local door_tile_positions = {
    {13, 7},  -- right
    {7, 1},   -- up
    {1, 7},   -- left
    {7, 13},   -- down
}

-- door sprite dimensions in tiles {cols, rows}
local door_dims = {
    {2, 2},  -- right
    {2, 2},  -- up
    {2, 2},  -- left
    {2, 2},  -- down
}

-- tile variants for randomization

-- floor
local floor_tiles = {52, 49, 73, 97, 121, 145}

-- walls (TODO: add more tile variants)
local wall_tiles = {
    {54},  -- [1] right outer  (x=14)
    {53},  -- [2] right inner  (x=13)
    {4},   -- [3] top outer    (y=1)
    {28},  -- [4] top inner    (y=2)
    {50},  -- [5] left outer   (x=1)
    {51},  -- [6] left inner   (x=2)
    {100}, -- [7] bottom outer (y=14)
    {76},  -- [8] bottom inner (y=13)
}

local wall_configs = {
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 14, tile_idx = 1, is_vertical = true},  -- right outer
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 13, tile_idx = 2, is_vertical = true},  -- right inner
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 1,  tile_idx = 3, is_vertical = false}, -- top outer
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 2,  tile_idx = 4, is_vertical = false}, -- top inner
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 1,  tile_idx = 5, is_vertical = true},  -- left outer
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 2,  tile_idx = 6, is_vertical = true},  -- left inner
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 14, tile_idx = 7, is_vertical = false}, -- bottom outer
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 13, tile_idx = 8, is_vertical = false}, -- bottom inner
}

local function is_floor_tile(tile_id)
    for _, floor_tile in ipairs(floor_tiles) do
        if tile_id == floor_tile then
            return true
        end
    end
    return false
end

local base_wall_tiles = {54, 53, 4, 28, 50, 51, 100, 76}  -- right outer/inner, top outer/inner, left outer/inner, bottom outer/inner

local next_object_id = 1000 -- unique identifier for objects on this tilemap
-- TODO use the max(ID) of existing objects as a start? or is starting at 1000 always safe?

local function is_wall_tile(tile_id)
    for _, wall_tile in ipairs(base_wall_tiles) do
        if tile_id == wall_tile then
            return true
        end
    end
    return false
end

local function randomize_tiles(room_data)
    for _, layer in ipairs(room_data.layers) do
        if layer.name == "floor" then
            -- randomize floor tiles
            for i = 1, #layer.data do
                if is_floor_tile(layer.data[i]) then
                    layer.data[i] = floor_tiles[dungeon_random(#floor_tiles)]
                end
            end
            
        elseif layer.name == "walls" then
            local width = room_data.width
            
            -- randomize wall tiles based on wall_configs
            for _, config in ipairs(wall_configs) do
                for _, pos in ipairs(config.pos_range) do
                    if dungeon_random() < 0.4 then
                        local wall_x, wall_y
                        if config.is_vertical then
                            wall_x = config.fixed_pos
                            wall_y = pos
                        else
                            wall_x = pos
                            wall_y = config.fixed_pos
                        end
                        
                        local wall_idx = wall_x + wall_y * width + 1
                        
                        -- only randomize base wall tiles (not doors)
                        if is_wall_tile(layer.data[wall_idx]) then
                            local variants = wall_tiles[config.tile_idx]
                            layer.data[wall_idx] = variants[dungeon_random(#variants)]
                        end
                    end
                end
            end
        end
    end
end

local function normalize_sparse_layers(room)
    -- fixes some lua quirks with arrays and tables
    local width = room.width
    local height = room.height
    local total = width * height

    for _, layer in ipairs(room.layers) do
        if layer.type == "tilelayer" and layer.data then
            -- if #layer.data == 0, it's a non-sequence table (sparse format)
            if #layer.data == 0 then
                local dense = {}
                for i = 1, total do
                    dense[i] = layer.data[i] or 0
                end
                layer.data = dense
            end
        end
    end
end

local function set_room_id(room, id)
    if not room.properties then
        room.properties = {}
    end
    table.insert(room.properties, {name = "roomID", type = "string", value = id})
end

local function load_tilemap(tilemap_base_path, doors, randomize)
    local tilemap_path = tilemap_base_path .. "/room_" .. doors .. ".json"
    local file = io.open(tilemap_path, "r")
    if not file then
        error("Could not open tilemap: " .. tilemap_path)
    end
    local content = file:read("*all")
    file:close()
    local room = json.decode(content)

    normalize_sparse_layers(room)

    if randomize then
        randomize_tiles(room)
    end

    return room
end

-- ============================================================================
-- Room Processing Functions
-- ============================================================================

local function find_locked_edges(dungeon_data, level_idx, room_coords)
    local locked_edges = {}
    local level_data = dungeon_data.levels[level_idx + 1]
    
    for _, edge in ipairs(level_data.edges) do
        if edge.required_items and #edge.required_items > 0 then
            local from = edge.from
            local to = edge.to
            
            local row_delta, col_delta

            -- check if current room is 'from' (direction to 'to')
            if from[1] == room_coords[1] and from[2] == room_coords[2] then
                row_delta = to[1] - from[1]
                col_delta = to[2] - from[2]
            -- check if current room is 'to' (direction to 'from')
            elseif to[1] == room_coords[1] and to[2] == room_coords[2] then
                row_delta = from[1] - to[1]
                col_delta = from[2] - to[2]
            else
                goto continue
            end

            -- determine direction index
            local direction_idx = nil
            if row_delta == 0 and col_delta == 1 then
                direction_idx = 0  -- right
            elseif row_delta == -1 and col_delta == 0 then
                direction_idx = 1  -- up
            elseif row_delta == 0 and col_delta == -1 then
                direction_idx = 2  -- left
            elseif row_delta == 1 and col_delta == 0 then
                direction_idx = 3  -- down
            end

            if direction_idx then
                table.insert(locked_edges, {
                    direction = direction_idx,
                    item = edge.required_items[1],
                    from_room = from,
                    to_room = to
                })
            end
            
            ::continue::
        end
    end
    
    return locked_edges
end

local function process_starting_room(tilemap_base_path, doors, tilemap_name, ObjectTemplates, level, row, col)
    -- add exit door (down = bit 3)
    local doors_num = tonumber(doors, 2)
    doors_num = doors_num | 1  -- set bit 0 for down door
    local modified_doors = ""
    for i = 3, 0, -1 do
        modified_doors = modified_doors .. tostring((doors_num >> i) & 1)
    end
    
    local room = load_tilemap(tilemap_base_path, modified_doors, true)
    
    -- add NPCs and teleport to overworld (TODO: modify teleport target based on the dungeon config)
    for _, layer in ipairs(room.layers) do
        if layer.name == "sprites" then
            table.insert(layer.objects, ObjectTemplates.create("elf_companion_1"))
            table.insert(layer.objects, ObjectTemplates.create("elf_companion_2"))
            table.insert(layer.objects, ObjectTemplates.create("teleport"))
            break
        end
    end
    
    return room, string.format("L%d_R%d_C%d_start.json", level, row, col)
end

local function process_item_room(tilemap_base_path, doors, tilemap_name, ObjectTemplates, level, row, col)
    local room = load_tilemap(tilemap_base_path, doors, true)
    
    -- add chest
    for _, layer in ipairs(room.layers) do
        if layer.name == "sprites" then
            table.insert(layer.objects, ObjectTemplates.create("chest"))
            break
        end
    end
    
    return room, string.format("L%d_R%d_C%d_item.json", level, row, col)
end

local function process_connection_room(tilemap_base_path, doors, tilemap_name, level_diff, ObjectTemplates, level, row, col)
    local room = load_tilemap(tilemap_base_path, doors, true)
    
    local stairs = ObjectTemplates.create("stairs")
    stairs.properties[1].value = level_diff
    stairs.properties[2].value = (level_diff > 0) and "ladder_up" or "ladder_down"
    
    -- center position
    local tilesize = room.tilewidth
    stairs.x = 9 * tilesize
    stairs.y = 10 * tilesize
    if level_diff > 0 then
        stairs.y = stairs.y - 32
    end
    
    -- add stairs
    for _, layer in ipairs(room.layers) do
        if layer.name == "sprites" then
            table.insert(layer.objects, stairs)
            break
        end
    end
    
    return room, string.format("L%d_R%d_C%d_connection.json", level, row, col)
end

local function process_normal_room(tilemap_base_path, doors, tilemap_name, level, row, col)
    local room = load_tilemap(tilemap_base_path, doors, true)
    return room, string.format("L%d_R%d_C%d.json", level, row, col)
end

local function add_combat_encounter_to_room(room, combat_locks, ObjectTemplates, level_idx, trigger_id)
    -- add enemy objects
    -- modify the door at the edge to be closed

    local tilesize = room.tilewidth
    local direction_names = {"right", "up", "left", "down"}

    for _, lock_info in ipairs(combat_locks) do
        closed_door = ObjectTemplates.create("closed_door")

        -- TODO modify enemies
        enemy = ObjectTemplates.create("enemy")


        print(string.format("placing an enemy (%s)", enemy.properties[2].value))
        
        -- get tile position
        local tile_pos = door_tile_positions[lock_info.direction + 1]
        local dims = door_dims[lock_info.direction + 1]

        -- convert to pixel position
        closed_door.x = tile_pos[1] * tilesize
        closed_door.y = tile_pos[2] * tilesize

        print(lock_info.direction + 1)
        print(string.format("placing a door (%s) at %f, %f", direction_names[lock_info.direction + 1], closed_door.x, closed_door.y))
        
        --[[
        -- create normalized event ID from edge coordinates
        -- TODO obsolete
        local r1, c1 = lock_info.from_room[1], lock_info.from_room[2]
        local r2, c2 = lock_info.to_room[1], lock_info.to_room[2]
        -- normalize: ensure smaller coordinates come first for consistency
        if r1 > r2 or (r1 == r2 and c1 > c2) then
            r1, c1, r2, c2 = r2, c2, r1, c1
        end
        local event_id = string.format("door_%d_%d_%d_%d_%d_opened", level_idx, r1, c1, r2, c2)
        ]]
        
        -- set properties by name
        for _, prop in ipairs(closed_door.properties) do
            if prop.name == "direction" then
                prop.value = lock_info.direction
            elseif prop.name == "event" then
                prop.value = trigger_id
            end
        end
        
        -- assign a unique ID
        closed_door.id = next_object_id
        next_object_id = next_object_id + 1

        -- add door and enemies to sprites layer
        for _, layer in ipairs(room.layers) do
            if layer.name == "sprites" then
                table.insert(layer.objects, closed_door)
                -- TODO just adding 1 enemy for testing
                table.insert(layer.objects, enemy)
                break
            end
        end
    end


end

local function add_locked_doors_to_room(room, locked_doors, ObjectTemplates, level_idx)
    -- for doors locked with a key
    local tilesize = room.tilewidth
    local direction_names = {"right", "up", "left", "down"}
    
    for _, lock_info in ipairs(locked_doors) do
        local locked_door
        if lock_info.item == "key" then
            locked_door = ObjectTemplates.create("locked_door")
        elseif lock_info.item == "boss_key" then
            locked_door = ObjectTemplates.create("boss_door")
            -- TODO change the door tiles on the top layer
        else
            print("ERROR: No object template for" .. lock_info.item )
            return
        end

        print(string.format("placing a door (%s)", direction_names[lock_info.direction + 1]))
        
        -- get tile position
        local tile_pos = door_tile_positions[lock_info.direction + 1]
        local dims = door_dims[lock_info.direction + 1]
        
        -- convert to pixel position 
        locked_door.x = tile_pos[1] * tilesize
        locked_door.y = tile_pos[2] * tilesize
        
        -- create normalized event ID from edge coordinates
        local r1, c1 = lock_info.from_room[1], lock_info.from_room[2]
        local r2, c2 = lock_info.to_room[1], lock_info.to_room[2]
        -- normalize: ensure smaller coordinates come first for consistency
        if r1 > r2 or (r1 == r2 and c1 > c2) then
            r1, c1, r2, c2 = r2, c2, r1, c1
        end
        local event_id = string.format("door_%d_%d_%d_%d_%d_unlocked", level_idx, r1, c1, r2, c2)
        
        -- set properties by name
        for _, prop in ipairs(locked_door.properties) do
            if prop.name == "direction" then
                prop.value = lock_info.direction
            elseif prop.name == "event" then
                prop.value = event_id
            end
        end
        
        -- assign a unique ID
        locked_door.id = next_object_id
        next_object_id = next_object_id + 1

        -- add to sprites layer
        for _, layer in ipairs(room.layers) do
            if layer.name == "sprites" then
                table.insert(layer.objects, locked_door)
                break
            end
        end
    end
end

-- ============================================================================
-- Main Processing Function
-- ============================================================================

function TilemapModifier.process_dungeon(dungeon_json_path, dungeon_name, tilemap_base_path, output_path, ObjectTemplates, event_triggers_path)
    -- load event triggers
    local triggers_file = io.open(event_triggers_path, "r")
    if not triggers_file then
        error("Could not open event triggers file: " .. event_triggers_path)
    end
    local triggers_content = triggers_file:read("*all")
    triggers_file:close()
    local event_triggers = json.decode(triggers_content)

    -- load dungeon data
    local file = io.open(dungeon_json_path, "r")
    if not file then
        error("Could not open dungeon file: " .. dungeon_json_path)
    end
    local content = file:read("*all")
    file:close()
    local all_dungeons = json.decode(content)
    
    -- get the specific dungeon
    local dungeon_data = all_dungeons[dungeon_name]
    if not dungeon_data then
        error("Dungeon not found: " .. dungeon_name)
    end
    
    local starting_level = dungeon_data.starting_level
    local starting_room = dungeon_data.starting_room
    
    -- process each level
    for level_idx = 0, #dungeon_data.levels - 1 do
        local level = dungeon_data.levels[level_idx + 1]
        
        for _, room_data in ipairs(level.rooms) do
            local room_coords = {room_data.row, room_data.column}
            local doors = room_data.doors
            local tilemap_name = room_data.tilemap

            print(string.format("processing level %d (%d,%d): %s", level_idx, room_data.row, room_data.column, tilemap_name))
            
            local room, filename
            
            -- check if starting room
            if level_idx == starting_level and 
               room_coords[1] == starting_room[1] and 
               room_coords[2] == starting_room[2] then
                
                room, filename = process_starting_room(tilemap_base_path, doors, tilemap_name, ObjectTemplates, level_idx, room_data.row, room_data.column)
            
            -- check if item room
            elseif room_data.item then
                room, filename = process_item_room(tilemap_base_path, doors, tilemap_name, ObjectTemplates, level_idx, room_data.row, room_data.column)
            
            -- check if stairway room
            else
                local level_diff = nil
                for _, conn in ipairs(dungeon_data.level_connections) do
                    if (conn.from == level_idx or conn.to == level_idx) and
                       conn.room[1] == room_coords[1] and 
                       conn.room[2] == room_coords[2] then
                        
                        level_diff = (conn.from == level_idx) and (conn.to - conn.from) or (conn.from - conn.to)
                        break
                    end
                end
                
                if level_diff then
                    room, filename = process_connection_room(tilemap_base_path, doors, tilemap_name, level_diff, ObjectTemplates, level_idx, room_data.row, room_data.column)
                else
                    room, filename = process_normal_room(tilemap_base_path, doors, tilemap_name, level_idx, room_data.row, room_data.column)
                end
            end

            local room_id = filename:gsub("%.json$", "")
            set_room_id(room, room_id)

            -- update elf sword cutscene trigger to match starting room ID
            if level_idx == starting_level and
               room_coords[1] == starting_room[1] and
               room_coords[2] == starting_room[2] then
                for _, trigger in ipairs(event_triggers.triggers) do
                    if trigger.id == "elf_sword_cutscene" then
                        trigger.conditions.roomID = room_id
                        break
                    end
                end
            end

            -- check for locked edges
            local locked_edges = find_locked_edges(dungeon_data, level_idx, room_coords)
            if #locked_edges > 0 then
                -- separate by requirement type
                local key_locks = {}
                local combat_locks = {}
                -- TODO add more lock types
    
                for _, edge_info in ipairs(locked_edges) do
                    print(string.format("Edge from [%d,%d] to [%d,%d] is locked by %s", 
                        edge_info.from_room[1], 
                        edge_info.from_room[2], 
                        edge_info.to_room[1], 
                        edge_info.to_room[2], 
                        tostring(edge_info.item)))
                    if edge_info.item == "key" or edge_info.item == "boss_key" then
                        table.insert(key_locks, edge_info)
                    elseif edge_info.item == "weapon_sword" then
                        -- sword locks are one-directional: the fight is only placed in the "from" room of this edge
                        if edge_info.from_room[1] == room_coords[1] and edge_info.from_room[2] == room_coords[2] then
                            table.insert(combat_locks, edge_info)
                        end
                    end
                    -- TODO other locks
                end
    
                -- handle each type with specific function
                if #key_locks > 0 then
                    add_locked_doors_to_room(room, key_locks, ObjectTemplates, level_idx)
                end
    
                -- TODO: implement later
                if #combat_locks > 0 then
                    local trigger_id = room_id .. "_enemies_defeated"

                    add_combat_encounter_to_room(room, combat_locks, ObjectTemplates, level_idx, trigger_id)

                    -- define the trigger
                    local already_exists = false
                    for _, trigger in ipairs(event_triggers.triggers) do
                        if trigger.id == trigger_id then
                            already_exists = true
                            break
                        end
                    end
                    if not already_exists then
                        table.insert(event_triggers.triggers, {
                            id = trigger_id,
                            script = "scripts/cutscenes/enemies_defeated.lua",
                            conditions = {
                                roomID = room_id,
                                noEnemies = true,
                                maxRoomState = 1
                            }
                        })
                    end
                end
            end
            
            -- update tilemap reference in dungeon data
            room_data.tilemap = filename:gsub("%.json$", "")
            
            -- save modified tilemap
            local output_file = io.open(output_path .. "/" .. filename, "w")
            if not output_file then
                error("Could not write tilemap: " .. output_path .. "/" .. filename)
            end
            output_file:write(json.encode(room, 2))
            output_file:close()
        end
    end
    
    -- save updated event triggers
    local triggers_out = io.open(event_triggers_path, "w")
    triggers_out:write(json.encode(event_triggers, 2))
    triggers_out:close()

    -- save updated dungeon data
    all_dungeons[dungeon_name] = dungeon_data
    local dungeon_out = io.open(dungeon_json_path, "w")
    dungeon_out:write(json.encode(all_dungeons, 2))
    dungeon_out:close()
    
    print("Tilemap processing complete!")
end

return TilemapModifier