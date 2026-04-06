local OW_Metatiles = require("overworld.ow_metatiles")
local utils = require("lib.utils")
local WorldUtils = require("lib.world_utils")
local GenerateBaseRooms = {}

-- base tilemap (room size is important)
local BASE_TILEMAP_PATH = "resources/tilemaps/base/overworld/fields_empty.json"


local DIRECTIONS = {
    n = { dr = -1, dc = 0 },
    s = { dr = 1, dc = 0 },
    e = { dr = 0, dc = 1 },
    w = { dr = 0, dc = -1 },
}

local CORNER_COMBOS = {
    {a = "n", b = "w", corner = "nw"},
    {a = "n", b = "e", corner = "ne"},
    {a = "s", b = "w", corner = "sw"},
    {a = "s", b = "e", corner = "se"},
}


-- helper functions
local function build_edge_set(edges)
    local set = {}
    for _, edge in ipairs(edges) do
        set[edge.from .. "->" .. edge.to] = edge.requirements or {}
    end
    return set
end

local function are_connected(edge_set, from_row, from_col, to_row, to_col)
    -- check if this edge exists
    local key = WorldUtils.node_name(from_row, from_col) .. "->" .. WorldUtils.node_name(to_row, to_col)
    return edge_set[key] ~= nil
end

local function get_neighbor_zone(zone_grid, row, col, dr, dc, grid_width, grid_height)
    local neighbor_row = row + dr
    local neighbor_col = col + dc
    if neighbor_row < 0 or neighbor_row >= grid_height or neighbor_col < 0 or neighbor_col >= grid_width then
        -- neighbor is outside the world grid
        return "boundary"
    end
    return zone_grid[neighbor_row][neighbor_col]
end



-- main function

function GenerateBaseRooms.process_overworld(zone_grid, edges, grid_width, grid_height)
    assert(zone_grid, "process_overworld: zone_grid is nil")
    assert(edges, "process_overworld: edges is nil")

    filesystem.create_directory("resources/tilemaps/generated/overworld")

    -- store a lookup table of the graph's edges
    local edge_set = build_edge_set(edges)

    -- make a room_states table that saves the adjacent zones for easy lookup
    -- TODO seems too bloated
    --[[
    local room_states = {}
    for row = 0, grid_height - 1 do
        room_states[row] = {}
        for col = 0, grid_width - 1 do
            local my_zone = zone_grid[row][col]
            local sides = {}
            for side, dir in pairs(DIRECTIONS) do
                local neighbor_zone = get_neighbor_zone(zone_grid, row, col, dir.dr, dir.dc, grid_width, grid_height)
                local is_closed = neighbor_zone == "boundary" or not are_connected(edge_set, row, col, row + dir.dr, col + dir.dc)
                local has_border = is_closed and (border_chunks[my_zone] and border_chunks[my_zone][neighbor_zone] or false)
                local has_transition = not is_closed and (transition_chunks[my_zone] and transition_chunks[my_zone][neighbor_zone] or false)
                sides[side] = has_border or has_transition
            end
            room_states[row][col] = sides
        end
    end

    ]]

    local base_tilemap = utils.loadJSON(BASE_TILEMAP_PATH)

    for row = 0, grid_height - 1 do
        for col = 0, grid_width - 1 do
            -- determine the current zone
            local my_zone = zone_grid[row][col]
            local base_metatile_key = my_zone .. "_base_tile"

            local dst_map = utils.deep_copy(base_tilemap)
            local metatiles_w = dst_map.width / 5
            local metatiles_h = dst_map.height / 5

            print("metatiles_w: " .. metatiles_w)
            print("metatiles_h: " .. metatiles_h)

            -- fill in the correct base tiles (sized 5x5)
            for x = 0, metatiles_w - 1 do
                for y = 0, metatiles_h - 1 do
                    OW_Metatiles.place_metatile(base_metatile_key, dst_map, x * 5, y * 5)
                end
            end

            -- handle edges to other rooms
            local boundary_sides = {}
            for side, dir in pairs(DIRECTIONS) do
                local neighbor_zone = get_neighbor_zone(zone_grid, row, col, dir.dr, dir.dc, grid_width, grid_height)
                if neighbor_zone == "boundary" then
                    boundary_sides[side] = true
                end
            end


            for side, _ in pairs(boundary_sides) do
                local key = my_zone .. "_boundary_" .. side
                -- get data because metatiles can have varying sizes depending on the zone
                local mt_data = OW_Metatiles.get_metatile_data(key)
                local count_x = dst_map.width / mt_data.w
                local count_y = dst_map.height / mt_data.h
                local edge_info = {
                    n = {count = count_x, step_x = mt_data.w, step_y = 0, start_x = 0, start_y = 0},
                    s = {count = count_x, step_x = mt_data.w, step_y = 0, start_x = 0, start_y = dst_map.height - mt_data.h},
                    w = {count = count_y, step_x = 0, step_y = mt_data.h, start_x = 0, start_y = 0},
                    e = {count = count_y, step_x = 0, step_y = mt_data.h, start_x = dst_map.width - mt_data.w, start_y = 0},
                }
                local e = edge_info[side]
                for i = 0, e.count - 1 do
                    OW_Metatiles.place_metatile(key, dst_map, e.start_x + i * e.step_x, e.start_y + i * e.step_y)
                end
            end

            for _, combo in ipairs(CORNER_COMBOS) do
                if boundary_sides[combo.a] and boundary_sides[combo.b] then
                    local key = my_zone .. "_boundary_corner_" .. combo.corner
                    -- paste meta onto tilemap at the corner position
                end
            end
            
            -- ### save the modified map ###
            -- change the tileset path to only the basename (game crashes otherwise)
            for _, ts in ipairs(dst_map.tilesets) do
                ts.source = utils.basename(ts.source)
            end

            local out_path = string.format("resources/tilemaps/generated/overworld/%s.json", WorldUtils.node_name(row, col))
            utils.saveJSON(out_path, dst_map)
            print(string.format("  saved %s (%s)", out_path, my_zone))
        end
    end


    print("process_overworld: done")
end

return GenerateBaseRooms