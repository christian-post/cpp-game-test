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

local OPPOSITE_SIDES = {
    n = "s", s = "n", w = "e", e = "w"
}

local CORNER_COMBOS = {
    {a = "n", b = "w", corner = "nw"},
    {a = "n", b = "e", corner = "ne"},
    {a = "s", b = "w", corner = "sw"},
    {a = "s", b = "e", corner = "se"},
}

local border_metatiles = {
    field = {
        forest = {
            outer_corner_key = "forest_outer_corner_",
            inner_corner_key = "forest_inner_corner_",
            edge_key = "forest_edge_straight_"
        },
        mountain = {
            outer_corner_key = "field_mountain_outer_corner_",
            inner_corner_key = "field_boundary_corner_",
            edge_key = "field_mountain_edge_straight_"
        },
        lake = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = "",
        },
        town = nil,
        boundary = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = "field_boundary_"
        },
    },
    forest = {
        field = {
            outer_corner_key = "",
            inner_corner_key = "forest_inner_corner_",
            edge_key = "forest_inner_edge_straight_"
        },
        mountain = {
            outer_corner_key = "",
            inner_corner_key = "forest_inner_corner_",
            edge_key = ""
        },
        town = {
            outer_corner_key = "",
            inner_corner_key = "forest_inner_corner_",
            edge_key = ""
        },
        lake = {
            outer_corner_key = "",
            inner_corner_key = "forest_inner_corner_",
            edge_key = "forest_inner_edge_straight_"
        },
        boundary = {
            outer_corner_key = "",
            inner_corner_key = "forest_inner_corner_",
            edge_key = "forest_inner_edge_straight_"
        },
    },
    mountain = {
        field = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = "",
            boundary_transition_key = ""
        },
        town = nil,
        lake = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = "",
            boundary_transition_key = ""
        },
        forest = nil,
        mountain = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = ""
        },
        boundary = {
            outer_corner_key = "",
            inner_corner_key = "mountain_boundary_corner_",
            edge_key = "mountain_boundary_"
        },
    },
    lake = {
        field = {
            outer_corner_key = "",
            inner_corner_key = "lake_inner_corner_w_margin_",
            edge_key = "lake_field_edge_straight_",
            boundary_transition_key = "lake_field_boundary_transition_"
        },
        town = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = "lake_field_edge_straight_"
        },
        forest = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = "lake_forest_edge_straight_"
        },
        mountain = nil,
        boundary = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = "lake_boundary_"
        },
    },
    town = {
        field = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = ""
        },
        forest = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = ""
        },
        mountain = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = ""
        },
        lake = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = ""
        },
        boundary = {
            outer_corner_key = "",
            inner_corner_key = "",
            edge_key = "town_boundary_"
        },
    }
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

    local base_tilemap = utils.loadJSON(BASE_TILEMAP_PATH)

    -- table similar to zone_grid, but it stores which edges got placed
    -- this is needed in a second pass that places edge connections (outer corners) etc.
    adjacent_edges_grid = {}
    for row = 0, grid_height - 1 do
        adjacent_edges_grid[row]= {}
        for col = 0, grid_width - 1 do
            adjacent_edges_grid[row][col] = {}
        end
    end

    dst_maps = {}
    for row = 0, grid_height - 1 do
        dst_maps[row] = {}
    end

    -- first pass 
    for row = 0, grid_height - 1 do
        for col = 0, grid_width - 1 do
            -- determine the current zone
            local my_zone = zone_grid[row][col]
            local base_metatile_key = my_zone .. "_base_tile"

            local dst_map = utils.deep_copy(base_tilemap)
            local metatiles_w = dst_map.width / 5
            local metatiles_h = dst_map.height / 5

            -- fill in the correct base tiles (sized 5x5)
            for x = 0, metatiles_w - 1 do
                for y = 0, metatiles_h - 1 do
                    OW_Metatiles.place_metatile(base_metatile_key, dst_map, x * 5, y * 5)
                end
            end

            print("\n### Row: " .. row .. ", Col: " .. col .. ", zone: " .. my_zone)

            -- handle edges to other rooms and the boundary

            -- collect neighbor side types
            -- TODO get rid of "boundary_sides and use adjacent_zones instead
            local boundary_sides = {}
            local adjacent_zones = {}
            --local adjacent_edges = {} -- store which neighbors have straight edges, to place outer corners
            for side, dir in pairs(DIRECTIONS) do
                local neighbor_zone = get_neighbor_zone(zone_grid, row, col, dir.dr, dir.dc, grid_width, grid_height)
                if neighbor_zone == "boundary" then
                    boundary_sides[side] = true
                end
                adjacent_zones[side] = neighbor_zone
            end

            -- boundary straight edges
            for side, _ in pairs(boundary_sides) do
                --local key = my_zone .. "_boundary_" .. side
                local key = border_metatiles[my_zone].boundary.edge_key .. side
                -- get data because metatiles can have varying sizes depending on the zone
                local mt_data = OW_Metatiles.get_metatile_data(key)
                local count_x = dst_map.width / mt_data.w
                local count_y = dst_map.height / mt_data.h
                -- compile info about the metatiles that need to be pasted
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

            -- boundary corners
            for _, combo in ipairs(CORNER_COMBOS) do
                if boundary_sides[combo.a] and boundary_sides[combo.b] then
                    local key = my_zone .. "_boundary_corner_" .. combo.corner
                    -- paste meta onto tilemap at the corner position
                    mt_data = OW_Metatiles.get_metatile_data(key)
                    -- make sure the tile gets pasted snug into the corner
                    local corner_info = {
                        nw = { start_x = 0, start_y = 0 },
                        ne = { start_x = dst_map.width - mt_data.w, start_y = 0 },
                        sw = { start_x = 0, start_y = dst_map.height - mt_data.h },
                        se = { start_x = dst_map.width - mt_data.w, start_y = dst_map.height - mt_data.h },
                    }
                    OW_Metatiles.place_metatile(key, dst_map, corner_info[combo.corner].start_x, corner_info[combo.corner].start_y)
                end
            end

            -- adjacent zones
            for side, other_zone in pairs(adjacent_zones) do
                print("side: ".. side .. ", other zone: " .. other_zone)

                local keys = border_metatiles[my_zone][other_zone]
                -- straight edges
                if keys == nil or keys.edge_key == nil or keys.edge_key == "" then
                    --print("skipping edge from " .. my_zone .. " to " .. other_zone .. " at " .. side)
                    print("-- skipping this edge")
                    goto continue
                end

                local edge_key = keys.edge_key .. side
                mt_straight_edge_data = OW_Metatiles.get_metatile_data(edge_key)
                local count_x = dst_map.width / mt_straight_edge_data.w
                local count_y = dst_map.height / mt_straight_edge_data.h
                local edge_placement_info = {
                    n = {count = count_x, step_x = mt_straight_edge_data.w, step_y = 0, start_x = 0, start_y = 0},
                    s = {count = count_x, step_x = mt_straight_edge_data.w, step_y = 0, start_x = 0, start_y = dst_map.height - mt_straight_edge_data.h},
                    w = {count = count_y, step_x = 0, step_y = mt_straight_edge_data.h, start_x = 0, start_y = 0},
                    e = {count = count_y, step_x = 0, step_y = mt_straight_edge_data.h, start_x = dst_map.width - mt_straight_edge_data.w, start_y = 0},
                }
                local e = edge_placement_info[side]
                for i = 0, e.count - 1 do
                    OW_Metatiles.place_metatile(edge_key, dst_map, e.start_x + i * e.step_x, e.start_y + i * e.step_y)
                end

                -- add adjacent_edges_grid info
                -- TODO are the offsets row and col correct?
                -- TODO logic!
                if other_zone ~= "boundary" then
                    local opp_side = OPPOSITE_SIDES[side]
                    local dir = DIRECTIONS[side]
                    --adjacent_edges_grid[row + dir.dr][col + dir.dc][side] = { opp_side = other_zone }
                    adjacent_edges_grid[row][col][side] = { side = other_zone }
                    print("this zone (" .. my_zone .. ") borders on a different zone (".. other_zone .. ") to the " .. side .. " which has a closed border to the " .. opp_side)
                end

                ::continue::
            end

            print("\n== inner corners ==")
       
            for _, combo in ipairs(CORNER_COMBOS) do
                -- process the inner corners between adjacent zones
                local zone_a = adjacent_zones[combo.a]
                local zone_b = adjacent_zones[combo.b]
                -- check if the zones are different on both sides
                if zone_a ~= my_zone and zone_b ~= my_zone then
                    if zone_a == zone_b then
                        -- same zone on both sides of the corner
                        local keys = border_metatiles[my_zone][zone_a]
                        -- check if a metatile is needed
                        if keys == nil or keys.inner_corner_key == nil or keys.inner_corner_key == "" then
                            --print("skipping edge from " .. my_zone .. " to " .. other_zone .. " at " .. side)
                            print("-- skipping this inner corner because key is nil or keys.inner_corner_key is empty or nil." )
                            goto continue
                        end

                        local corner_key = keys.inner_corner_key .. combo.corner
                        mt_corner_data = OW_Metatiles.get_metatile_data(corner_key)
                        print("got inner corner data for " .. corner_key)

                        local corner_info = {
                            nw = { start_x = 0, start_y = 0 },
                            ne = { start_x = dst_map.width - mt_corner_data.w, start_y = 0 },
                            sw = { start_x = 0, start_y = dst_map.height - mt_corner_data.h },
                            se = { start_x = dst_map.width - mt_corner_data.w, start_y = dst_map.height - mt_corner_data.h },
                        }
                        -- clip the objects that are here from previous edge placements
                        -- for the erase_object_region function the coordinates need to be converted from tiles to pixels
                        local place_x = corner_info[combo.corner].start_x * 16
                        local place_y = corner_info[combo.corner].start_y * 16
                        local place_w = mt_corner_data.w * 16
                        local place_h = mt_corner_data.h * 16
                        print("erasing objects here: x1 = " .. place_x .. ", y1 = " .. place_y ..  ", x2 = " .. place_x + place_w .. ", y2 = " .. place_y + place_h)
                        OW_Metatiles.erase_object_region("static_collision", place_x, place_y, place_x + place_w, place_y + place_h, dst_map)
                        -- place the corner tile
                        OW_Metatiles.place_metatile(corner_key, dst_map, corner_info[combo.corner].start_x, corner_info[combo.corner].start_y, true, true)

                    elseif zone_a == "boundary" and zone_b == "boundary" then
                        -- handle case where both sides are the boundary (boundary.inner_corner_key)
                        local keys = border_metatiles[my_zone].boundary
                        if keys == nil or keys.inner_corner_key == nil or keys.inner_corner_key == "" then
                            --print("skipping edge from " .. my_zone .. " to " .. other_zone .. " at " .. side)
                            print("-- skipping this boundary corner because key is nil or keys.inner_corner_key is empty or nil." )
                            goto continue
                        end
                        local corner_key = keys.inner_corner_key .. combo.corner
                        mt_corner_data = OW_Metatiles.get_metatile_data(corner_key)
                        print("got boundary corner data for " .. corner_key)

                        local corner_info = {
                            nw = { start_x = 0, start_y = 0 },
                            ne = { start_x = dst_map.width - mt_corner_data.w, start_y = 0 },
                            sw = { start_x = 0, start_y = dst_map.height - mt_corner_data.h },
                            se = { start_x = dst_map.width - mt_corner_data.w, start_y = dst_map.height - mt_corner_data.h },
                        }
                        -- clip the objects that are here from previous edge placements
                        -- for the erase_object_region function the coordinates need to be converted from tiles to pixels
                        local place_x = corner_info[combo.corner].start_x * 16
                        local place_y = corner_info[combo.corner].start_y * 16
                        local place_w = mt_corner_data.w * 16
                        local place_h = mt_corner_data.h * 16
                        print("erasing objects here: x1 = " .. place_x .. ", y1 = " .. place_y ..  ", x2 = " .. place_x + place_w .. ", y2 = " .. place_y + place_h)
                        OW_Metatiles.erase_object_region("static_collision", place_x, place_y, place_x + place_w, place_y + place_h, dst_map)
                        -- place the corner tile
                        OW_Metatiles.place_metatile(corner_key, dst_map, corner_info[combo.corner].start_x, corner_info[combo.corner].start_y, true, true)

                    elseif zone_a == "boundary" and zone_b ~= my_zone or zone_b == "boundary" and zone_a ~=my_zone then
                        -- handle cases where one side is the boundary and the other side is a different zone
                        local other_zone = zone_a ~= "boundary" and zone_a or zone_b
                        print(my_zone .. ", " .. other_zone)
                        local keys = border_metatiles[my_zone][other_zone]
                        if keys == nil or keys.boundary_transition_key == nil or keys.boundary_transition_key == "" then
                            --print("skipping edge from " .. my_zone .. " to " .. other_zone .. " at " .. side)
                            print("-- skipping this zone transition because key is nil or keys.boundary_transition_key is empty or nil." )
                            goto continue
                        end

                        local boundary_key = keys.boundary_transition_key .. combo.corner
                        mt_corner_data = OW_Metatiles.get_metatile_data(boundary_key)
                        print("got zone transition tile data for " .. boundary_key)

                        local corner_info = {
                            nw = { start_x = 0, start_y = 0 },
                            ne = { start_x = dst_map.width - mt_corner_data.w, start_y = 0 },
                            sw = { start_x = 0, start_y = dst_map.height - mt_corner_data.h },
                            se = { start_x = dst_map.width - mt_corner_data.w, start_y = dst_map.height - mt_corner_data.h },
                        }
                        -- clip objects
                        local place_x = corner_info[combo.corner].start_x * 16
                        local place_y = corner_info[combo.corner].start_y * 16
                        local place_w = mt_corner_data.w * 16
                        local place_h = mt_corner_data.h * 16
                        print("erasing objects here: x1 = " .. place_x .. ", y1 = " .. place_y ..  ", x2 = " .. place_x + place_w .. ", y2 = " .. place_y + place_h)
                        OW_Metatiles.erase_object_region("static_collision", place_x, place_y, place_x + place_w, place_y + place_h, dst_map)
                        -- place tiles
                        OW_Metatiles.place_metatile(boundary_key, dst_map, corner_info[combo.corner].start_x, corner_info[combo.corner].start_y, true, true)
                    end

                    ::continue::
                end
            end
            -- outer corners get placed in a second pass


            -- TODO
            -- edge from forest to field/lake
            -- edge between two zones on the world boundary (special cases)
            -- corners between three zones
            -- open connections between zones (forest and mountain)

            dst_maps[row][col] = dst_map
        end
    end

    -- second pass: outer corners
    print("\n== outer corners ==")
    for row = 0, grid_height - 1 do
        for col = 0, grid_width - 1 do
            local dst_map = dst_maps[row][col]

            -- place outer corners using adjacent_edges_grid info
            local my_zone = zone_grid[row][col]
            local adjacent_edges = adjacent_edges_grid[row][col]
            for _, combo in ipairs(CORNER_COMBOS) do
                local dir_a = DIRECTIONS[combo.a]
                local dir_b = DIRECTIONS[combo.b]
                -- get neighbor data of both directions
                local neighbor_a_row = row + dir_a.dr
                local neighbor_a_col = col + dir_a.dc
                local neighbor_b_row = row + dir_b.dr
                local neighbor_b_col = col + dir_b.dc
                -- check out of bounds
                if neighbor_a_row < 0 or neighbor_a_row >= grid_height or neighbor_a_col < 0 or neighbor_a_col >= grid_width then
                    goto continue
                end
                if neighbor_b_row < 0 or neighbor_b_row >= grid_height or neighbor_b_col < 0 or neighbor_b_col >= grid_width then
                    goto continue
                end
                local neighbor_room_a = adjacent_edges_grid[neighbor_a_row][neighbor_a_col]
                local neighbor_room_b = adjacent_edges_grid[neighbor_b_row][neighbor_b_col]
                -- check if the neighbor rooms both have borders to the corner combo
                -- for example, if the corner is nw, check if the room to the west has a border to north,  
                -- and if the room to the north has a border to west
                if neighbor_room_a[combo.b] and neighbor_room_b[combo.a] then
                    print("there is an outer corner here (" .. combo.corner .. ") in row " .. row .. ", col " ..col)

                    -- place the correct tile

                    -- find out the diagonal neighbor's zone type
                    local diagonal_row = row + dir_a.dr + dir_b.dr
                    local diagonal_col = col + dir_a.dc + dir_b.dc
                    local other_zone = zone_grid[diagonal_row][diagonal_col]

                    local keys = border_metatiles[my_zone][other_zone]
                    local corner_key = keys.outer_corner_key .. combo.corner
                    mt_corner_data = OW_Metatiles.get_metatile_data(corner_key)
                    print("got outer corner data for " .. corner_key)
                    -- TODO repeated code
                    local corner_info = {
                        nw = { start_x = 0, start_y = 0 },
                        ne = { start_x = dst_map.width - mt_corner_data.w, start_y = 0 },
                        sw = { start_x = 0, start_y = dst_map.height - mt_corner_data.h },
                        se = { start_x = dst_map.width - mt_corner_data.w, start_y = dst_map.height - mt_corner_data.h },
                    }
                    -- clip the objects that are here from previous edge placements
                    -- for the erase_object_region function the coordinates need to be converted from tiles to pixels
                    local place_x = corner_info[combo.corner].start_x * 16
                    local place_y = corner_info[combo.corner].start_y * 16
                    local place_w = mt_corner_data.w * 16
                    local place_h = mt_corner_data.h * 16
                    print("erasing objects here: x1 = " .. place_x .. ", y1 = " .. place_y ..  ", x2 = " .. place_x + place_w .. ", y2 = " .. place_y + place_h)
                    OW_Metatiles.erase_object_region("static_collision", place_x, place_y, place_x + place_w, place_y + place_h, dst_map)
                    -- place the corner tile
                    OW_Metatiles.place_metatile(corner_key, dst_map, corner_info[combo.corner].start_x, corner_info[combo.corner].start_y, true)
                end

                ::continue::
            end

            dst_maps[row][col] = dst_map
        end
    end

    -- save all maps
    for row = 0, grid_height - 1 do
        for col = 0, grid_width - 1 do
            local dst_map = dst_maps[row][col]
            local my_zone = zone_grid[row][col]
            for _, ts in ipairs(dst_map.tilesets) do
                ts.source = utils.basename(ts.source)
            end
            local out_path = string.format("resources/tilemaps/generated/overworld/%s.json", WorldUtils.node_name(row, col))
            utils.saveJSON(out_path, dst_map)
            print(string.format("\n ==> saved %s (%s)", out_path, my_zone))
        end
    end


    print("process_overworld: done")
end

return GenerateBaseRooms