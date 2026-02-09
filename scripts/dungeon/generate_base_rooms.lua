local GenerateBaseRooms = {}

-- door tile configurations
local doors_floor = {
    {181, 201, 182, 202}, -- right (index 0)
    {103, 123, 104, 124}, -- up (index 1)
    {190, 210, 191, 211}, -- left (index 2)
    {148, 168, 149, 169}  -- down (index 3)
}

local doors_top = {
    {99, 119, 100, 120},   -- right (index 0)
    {19, 39, 20, 40},      -- up (index 1)
    {59, 79, 60, 80},      -- left (index 2)
    {139, 159, 140, 160}   -- down (index 3)
}

local door_positions = {
    {7, 14},  -- right (index 0)
    {0, 7},   -- up (index 1)
    {7, 0},   -- left (index 2)
    {14, 7}   -- down (index 3)
}

-- collision wall configurations
-- format: {door_index, solid_wall, segment1, segment2}
local collision_walls = {
    -- right wall
    {0,
     {height = 200, width = 28, x = 228, y = 28},
     {height = 84, width = 28, x = 228, y = 28},
     {height = 84, width = 28, x = 228, y = 144}},
    -- top wall
    {1, 
     {height = 28, width = 256, x = 0, y = 0},
     {height = 28, width = 112, x = 0, y = 0},
     {height = 28, width = 112, x = 144, y = 0}},
    
    -- left wall
    {2,
     {height = 200, width = 28, x = 0, y = 28},
     {height = 84, width = 28, x = 0, y = 28},
     {height = 84, width = 28, x = 0, y = 144}},
    
    -- bottom wall
    {3,
     {height = 28, width = 256, x = 0, y = 228},
     {height = 28, width = 112, x = 0, y = 228},
     {height = 28, width = 112, x = 144, y = 228}}
}

local function place_door_tiles(room_data, selected_doors)
    local width = room_data.width
    
    for _, layer in ipairs(room_data.layers) do
        if layer.name == "walls" then
            -- place door tiles in walls layer
            for _, door_idx in ipairs(selected_doors) do
                local row = door_positions[door_idx + 1][1]
                local col = door_positions[door_idx + 1][2]
                local door_tiles = doors_floor[door_idx + 1]
                
                -- convert 2D positions to 1D indices
                local idx_tl = row * width + col
                local idx_bl = (row + 1) * width + col
                local idx_tr = row * width + (col + 1)
                local idx_br = (row + 1) * width + (col + 1)
                
                -- place the door tiles (lua is 1-indexed)
                layer.data[idx_tl + 1] = door_tiles[1]  -- top-left
                layer.data[idx_bl + 1] = door_tiles[2]  -- bottom-left
                layer.data[idx_tr + 1] = door_tiles[3]  -- top-right
                layer.data[idx_br + 1] = door_tiles[4]  -- bottom-right
            end
            
        elseif layer.name == "top" then
            -- place door tiles in top layer
            for _, door_idx in ipairs(selected_doors) do
                local row = door_positions[door_idx + 1][1]
                local col = door_positions[door_idx + 1][2]
                local door_tiles = doors_top[door_idx + 1]
                
                local idx_tl = row * width + col
                local idx_bl = (row + 1) * width + col
                local idx_tr = row * width + (col + 1)
                local idx_br = (row + 1) * width + (col + 1)
                
                layer.data[idx_tl + 1] = door_tiles[1]
                layer.data[idx_bl + 1] = door_tiles[2]
                layer.data[idx_tr + 1] = door_tiles[3]
                layer.data[idx_br + 1] = door_tiles[4]
            end
        end
    end
end

local function setup_collision_walls(room_data, selected_doors)
    -- convert selected_doors array to set for fast lookup
    local selected_set = {}
    for _, door_idx in ipairs(selected_doors) do
        selected_set[door_idx] = true
    end
    
    -- find static_collision layer
    for _, layer in ipairs(room_data.layers) do
        if layer.name == "static_collision" then
            -- clear existing objects
            layer.objects = {}
            
            local object_id = 1
            
            -- add walls based on door configuration
            for _, wall_config in ipairs(collision_walls) do
                local door_idx = wall_config[1]
                local solid = wall_config[2]
                local segment1 = wall_config[3]
                local segment2 = wall_config[4]
                
                if not selected_set[door_idx] then
                    -- no door - make one solid wall
                    local wall = {
                        id = object_id,
                        name = "",
                        rotation = 0,
                        type = "wall",
                        visible = true,
                        height = solid.height,
                        width = solid.width,
                        x = solid.x,
                        y = solid.y
                    }
                    table.insert(layer.objects, wall)
                    object_id = object_id + 1
                else
                    -- door exists - keep two separate wall segments
                    local wall1 = {
                        id = object_id,
                        name = "",
                        rotation = 0,
                        type = "wall",
                        visible = true,
                        height = segment1.height,
                        width = segment1.width,
                        x = segment1.x,
                        y = segment1.y
                    }
                    table.insert(layer.objects, wall1)
                    object_id = object_id + 1
                    
                    local wall2 = {
                        id = object_id,
                        name = "",
                        rotation = 0,
                        type = "wall",
                        visible = true,
                        height = segment2.height,
                        width = segment2.width,
                        x = segment2.x,
                        y = segment2.y
                    }
                    table.insert(layer.objects, wall2)
                    object_id = object_id + 1
                end
            end
            
            break
        end
    end
end

function GenerateBaseRooms.generate(empty_tilemap_path, output_path)
    -- load empty tilemap template
    local file = io.open(empty_tilemap_path, "r")
    if not file then
        error("Could not open empty tilemap: " .. empty_tilemap_path)
    end
    local content = file:read("*all")
    file:close()
    local empty_tilemap = json.decode(content)
    
    -- generate all door combinations (1-15, excluding 0)
    for door_combination = 1, 15 do
        -- determine which doors are selected (0-indexed for array lookups)
        local selected_doors = {}
        for i = 0, 3 do
            if (door_combination & (1 << i)) ~= 0 then
                table.insert(selected_doors, i)
            end
        end
        
        -- create door string (e.g., "0110")
        local door_string = ""
        for i = 0, 3 do
            if (door_combination & (1 << i)) ~= 0 then
                door_string = door_string .. "1"
            else
                door_string = door_string .. "0"
            end
        end
        
        -- deep copy the empty tilemap
        local room_data = json.decode(json.encode(empty_tilemap))
        
        -- place door tiles
        place_door_tiles(room_data, selected_doors)
        
        -- setup collision walls
        setup_collision_walls(room_data, selected_doors)
        
        -- set tileset reference
        room_data.tilesets[1].source = "dungeon_topdown.tsj"
        
        -- save tilemap
        local output_file_path = output_path .. "/room_" .. door_string .. ".json"
        local out_file = io.open(output_file_path, "w")
        if not out_file then
            error("Could not write tilemap: " .. output_file_path)
        end
        out_file:write(json.encode(room_data, 2))
        out_file:close()
        
        print("Generated: room_" .. door_string .. ".json")
    end
    
    print("Base room generation complete!")
end

return GenerateBaseRooms