local utils = require("lib.utils")
local GenerateBaseRooms = {}

-- door tile configurations
-- IMPORTANT lua indices start at 1
local doors_floor = {
    { 55,  56,  79,  80, 103, 104, 127, 128}, -- right 
    {  7,   8,   9,  10,  31,  32,  33,  34}, -- left
    { 57,  58,  81,  82, 105, 106, 129, 130}, -- up
    {151, 152, 153, 154, 175, 176, 177, 178}  -- down
}

local doors_top = {
    { 80, 104},  -- right (index 0)
    {  8,   9},  -- up    (index 1)
    { 81, 105},  -- left  (index 2)
    {176, 177}   -- down  (index 3)
}

-- tells the program where the top tiles are in relation to the doors_floor tiles
local door_top_offsets = {
    {1, 1},  -- right: row+1, col+1
    {0, 1},  -- up:    row+0, col+1
    {1, 0},  -- left:  row+1, col+0
    {1, 1},  -- down:  row+1, col+1
}

local door_positions = {
    { 6, 13},
    { 1,  6},
    { 6,  1},
    {13, 6}
}

-- tile dimensions per door direction {cols, rows}
local door_floor_dims = {
    {2, 4},  -- right: 2 wide x 4 tall
    {4, 2},  -- up:    4 wide x 2 tall
    {2, 4},  -- left:  2 wide x 4 tall
    {4, 2},  -- down:  4 wide x 2 tall
}

local door_top_dims = {
    {1, 2},  -- right: 1 wide x 2 tall
    {2, 1},  -- up:    2 wide x 1 tall
    {1, 2},  -- left:  1 wide x 2 tall
    {2, 1},  -- down:  2 wide x 1 tall
}

-- collision wall configurations
-- format: {door_index, solid_wall, segment1, segment2}
-- solid wall = no door, segments = door in between
local collision_walls = {
    -- right wall
    {
        0,
        {height = 160, width = 48, x = 208, y = 48},
        {height = 68, width = 48, x = 208, y = 48},
        {height = 68, width = 48, x = 208, y = 140}
    },
    -- top wall
    {
        1, 
        {height = 48, width = 256, x = 0, y = 0},
        {height = 48, width = 116, x = 0, y = 0},
        {height = 48, width = 116, x = 140, y = 0}
    },
    
    -- left wall
    {
        2,
        {height = 160, width = 48, x = 0, y = 48},
        {height = 68, width = 48, x = 0, y = 48},
        {height = 68, width = 48, x = 0, y = 140}
    },
    
    -- bottom wall
    {
        3,
        {height = 48, width = 256, x = 0, y = 208},
        {height = 48, width = 116, x = 0, y = 208},
        {height = 48, width = 116, x = 140, y = 208}
    }
}

-- helper founction to write a rectangular block of tiles into a flat layer data array
local function place_tile_block(data, map_width, start_row, start_col, tiles, num_cols, num_rows)
    for r = 0, num_rows - 1 do
        for c = 0, num_cols - 1 do
            local tile_idx = r * num_cols + c + 1  -- index into tiles[]
            local map_idx  = (start_row + r) * map_width + (start_col + c) + 1  -- 1-based flat index
            data[map_idx] = tiles[tile_idx]
        end
    end
end

local function place_door_tiles(room_data, selected_doors)
    local width = room_data.width

    for _, layer in ipairs(room_data.layers) do
        for _, door_idx in ipairs(selected_doors) do
            local row  = door_positions[door_idx + 1][1]
            local col  = door_positions[door_idx + 1][2]

            if layer.name == "walls" then
                local dims = door_floor_dims[door_idx + 1]
                place_tile_block(layer.data, width, row, col,
                                 doors_floor[door_idx + 1], dims[1], dims[2])

            elseif layer.name == "top" then
                local dims = door_top_dims[door_idx + 1]
                local off  = door_top_offsets[door_idx + 1]
                place_tile_block(layer.data, width, row + off[1], col + off[2],
                                 doors_top[door_idx + 1], dims[1], dims[2])

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
    local empty_tilemap = utils.loadJSON(empty_tilemap_path)
    
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
        
        -- set tileset reference TODO: get from tiled data?
        room_data.tilesets[1].source = "simple_dungeon.tsj"
        
        -- save tilemap
        local output_file_path = output_path .. "/room_" .. door_string .. ".json"
        utils.saveJSON(output_file_path, room_data)
        
        print("Generated: room_" .. door_string .. ".json")
    end
    
    print("Base room generation complete!")
end

return GenerateBaseRooms