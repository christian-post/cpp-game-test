-- scripts/tilemap/tilemap_modifier.lua
local TilemapModifier = {}

local door_tile_positions = {
    {7, 14},  -- right (index 0)
    {0, 7},   -- up (index 1)
    {7, 0},   -- left (index 2)
    {14, 7}   -- down (index 3)
}

-- tile variants for randomization

local floor_tiles = {7, 8, 9, 10, 11, 12, 13, 27, 28, 29, 30, 31, 32, 33, 47, 48, 49, 50, 51, 52, 53}

local wall_tiles = {
    {82, 102, 122},  -- [1] right wall variants
    {61, 62, 63},    -- [2] top wall variants
    {81, 101, 121},  -- [3] left wall variants
    {83, 84, 85}     -- [4] bottom wall variants
}

local wall_configs = {
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 14, tile_idx = 1, is_vertical = true},   -- right: iterate y, fixed x=14
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 1, tile_idx = 2, is_vertical = false},   -- top: iterate x, fixed y=1
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 1, tile_idx = 3, is_vertical = true},    -- left: iterate y, fixed x=1
    {pos_range = {2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13}, fixed_pos = 14, tile_idx = 4, is_vertical = false}   -- bottom: iterate x, fixed y=14
}

local function is_floor_tile(tile_id)
    for _, floor_tile in ipairs(floor_tiles) do
        if tile_id == floor_tile then
            return true
        end
    end
    return false
end

local base_wall_tiles = {21, 42, 23, 2}  -- right, up, left, down

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


local function load_tilemap(tilemap_base_path, doors, randomize)
    local tilemap_path = tilemap_base_path .. "/room_" .. doors .. ".json"
    local file = io.open(tilemap_path, "r")
    if not file then
        error("Could not open tilemap: " .. tilemap_path)
    end
    local content = file:read("*all")
    file:close()
    local room = json.decode(content)

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
            
            local dr, dc
            
            -- check if current room is 'from' (direction to 'to')
            if from[1] == room_coords[1] and from[2] == room_coords[2] then
                dr = to[1] - from[1]
                dc = to[2] - from[2]
            
            -- check if current room is 'to' (direction to 'from')
            elseif to[1] == room_coords[1] and to[2] == room_coords[2] then
                dr = from[1] - to[1]
                dc = from[2] - to[2]
            else
                goto continue
            end
            
            -- determine direction index
            local direction_idx = nil
            if dr == 0 and dc == 1 then
                direction_idx = 0  -- right
            elseif dr == -1 and dc == 0 then
                direction_idx = 1  -- up
            elseif dr == 0 and dc == -1 then
                direction_idx = 2  -- left
            elseif dr == 1 and dc == 0 then
                direction_idx = 3  -- down
            end
            
            if direction_idx then
                table.insert(locked_edges, {
                    direction = direction_idx,
                    item = edge.required_items[1],
                    from_room = from,  -- edge info
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
    
    -- add roomID property
    if not room.properties then
        room.properties = {}
    end
    table.insert(room.properties, {
        name = "roomID",
        type = "string",
        value = "starting_room"
    })
    
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

local function add_combat_encounter_to_room(room, combat_locks, ObjectTemplates, level_idx)
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
        
        -- convert to pixel position (center of 2x2 door)
        closed_door.x = tile_pos[2] * tilesize
        closed_door.y = tile_pos[1] * tilesize
        
        -- create normalized event ID from edge coordinates
        local r1, c1 = lock_info.from_room[1], lock_info.from_room[2]
        local r2, c2 = lock_info.to_room[1], lock_info.to_room[2]
        -- normalize: ensure smaller coordinates come first for consistency
        if r1 > r2 or (r1 == r2 and c1 > c2) then
            r1, c1, r2, c2 = r2, c2, r1, c1
        end
        local event_id = string.format("door_%d_%d_%d_%d_%d_opened", level_idx, r1, c1, r2, c2)
        
        -- set properties by name
        for _, prop in ipairs(closed_door.properties) do
            if prop.name == "direction" then
                prop.value = lock_info.direction
            elseif prop.name == "event" then
                prop.value = event_id
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
        
        -- convert to pixel position (center of 2x2 door)
        locked_door.x = tile_pos[2] * tilesize
        locked_door.y = tile_pos[1] * tilesize
        
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

function TilemapModifier.process_dungeon(dungeon_json_path, dungeon_name, tilemap_base_path, output_path, ObjectTemplates)
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

            -- check for locked edges
            local locked_edges = find_locked_edges(dungeon_data, level_idx, room_coords)
            if #locked_edges > 0 then
                -- separate by requirement type
                local key_locks = {}
                local combat_locks = {}
                -- TODO add more lock types
    
                for _, edge_info in ipairs(locked_edges) do
                    print("Edge is locked by " .. edge_info.item)
                    if edge_info.item == "key" or edge_info.item == "boss_key" then
                        table.insert(key_locks, edge_info)
                    elseif edge_info.item == "weapon_sword" then
                        table.insert(combat_locks, edge_info)
                    end
                    -- TODO
                end
    
                -- handle each type with specific function
                if #key_locks > 0 then
                    add_locked_doors_to_room(room, key_locks, ObjectTemplates, level_idx)
                end
    
                -- TODO: implement later
                if #combat_locks > 0 then
                    add_combat_encounter_to_room(room, combat_locks, ObjectTemplates, level_idx)
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
    
    -- save updated dungeon data
    all_dungeons[dungeon_name] = dungeon_data
    local dungeon_out = io.open(dungeon_json_path, "w")
    dungeon_out:write(json.encode(all_dungeons, 2))
    dungeon_out:close()
    
    print("Tilemap processing complete!")
end

return TilemapModifier