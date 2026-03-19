-- adds overworld traversal features like rivers, bridges etc
local WorldUtils = require("lib.world_utils")
local OWFeatures = {}

-- ==================================
-- River
-- ==================================

local river_tiles = {
    origin = "resources/tilemaps/base/overworld/river_origin.json",
    straight_down = "resources/tilemaps/base/overworld/river_straight_down.json",
    straight_horizontal = "resources/tilemaps/base/overworld/river_straight_horizontal.json",
    left_turn_from_up = "resources/tilemaps/base/overworld/river_turn_west.json",
    right_turn_from_up = "resources/tilemaps/base/overworld/river_turn_east.json",
    down_from_left_turn = "resources/tilemaps/base/overworld/river_turn_south_from_east.json",
    down_from_right_turn = "resources/tilemaps/base/overworld/river_turn_south_from_west.json",
    down_into_lake = "resources/tilemaps/base/overworld/river_straight_down.json",
    --down_into_lake = "resources/tilemaps/base/overworld/river_down_into_lake.json",
    right_into_lake = "resources/tilemaps/base/overworld/river_right_into_lake.json",
    left_into_lake = "resources/tilemaps/base/overworld/river_left_into_lake.json",
}

local function generate_river(zone_grid, width, height)
    -- collect mountain cells in rows 0 and 1 as possible source positions
    local sources = {}
    for row = 1, height - 1 do
        for col = 0, width - 1 do
            if zone_grid[row][col] == "field" and zone_grid[row - 1][col] == "mountain" then
                table.insert(sources, {row = row, col = col})
            end
        end
    end
    if #sources == 0 then
        return nil
    end

    local src = sources[dungeon_random(#sources)]
    local path = {}
    local removed_edges = {}
    local visited = {}

    local function vis(r, c)
        if not visited[r] then
            visited[r] = {}
        end
        visited[r][c] = true
    end

    local function is_vis(r, c)
        return visited[r] and visited[r][c]
    end

    local function in_bounds(r, c)
        return r >= 0 and r < height and c >= 0 and c < width
    end

    local function zone_at(r, c)
        if not in_bounds(r, c) then
            return nil
        end
        return zone_grid[r][c]
    end

    local function can_walk(r, c)
        if not in_bounds(r, c) then
            return false
        end
        if is_vis(r, c) then
            return false
        end
        local z = zone_grid[r][c]
        return z ~= "town" and z ~= "lake"
    end

    -- removes horizontal traversal edges on both sides of cell (r, c)
    -- called only for cells where the river crosses north-to-south
    local function rm_horiz(r, c)
        if c > 0 then
            table.insert(removed_edges, {from = WorldUtils.node_name(r, c - 1), to = WorldUtils.node_name(r, c)})
            table.insert(removed_edges, {from = WorldUtils.node_name(r, c), to = WorldUtils.node_name(r, c - 1)})
        end
        if c < width - 1 then
            table.insert(removed_edges, {from = WorldUtils.node_name(r, c), to = WorldUtils.node_name(r, c + 1)})
            table.insert(removed_edges, {from = WorldUtils.node_name(r, c + 1), to = WorldUtils.node_name(r, c)})
        end
    end

    local function add_cell(r, c, tilemap)
        table.insert(path, {row = r, col = c, tilemaps = {{path = river_tiles[tilemap], overwrite = true}}})
    end

    local function add_cell2(r, c, base, overlay, right_cols)
        table.insert(path, {row = r, col = c, tilemaps = {
            {path = base, overwrite = true},
            {path = overlay, right_cols = right_cols, overwrite = true},
        }})
    end

    -- place the waterfall origin and start walking south
    vis(src.row, src.col)
    add_cell(src.row, src.col, "origin")
    rm_horiz(src.row, src.col)

    local row = src.row + 1
    local col = src.col
    local entry = "N"

    if not in_bounds(row, col) then
        return nil
    end
    if zone_at(row, col) == "lake" or zone_at(row, col) == "town" then
        return nil
    end

    for _ = 1, width * height do
        if is_vis(row, col) then
            return nil
        end
        vis(row, col)

        if entry == "N" then
            -- check lake neighbors for termination, south takes priority
            if zone_at(row + 1, col) == "lake" then
                add_cell(row, col, "down_into_lake")
                rm_horiz(row, col)
                return {path = path, removed_edges = removed_edges}
            end
            if zone_at(row, col + 1) == "lake" then
                add_cell2(row, col, river_tiles["right_turn_from_up"], river_tiles["right_into_lake"], 6)
                return {path = path, removed_edges = removed_edges}
            end
            if zone_at(row, col - 1) == "lake" then
                add_cell2(row, col, river_tiles["left_turn_from_up"], river_tiles["left_into_lake"], 6)
                return {path = path, removed_edges = removed_edges}
            end

            -- weighted random walk: south weight 3, east/west weight 1 each
            local choices = {}
            if can_walk(row + 1, col) then
                table.insert(choices, "S")
                table.insert(choices, "S")
                table.insert(choices, "S")
            end
            if can_walk(row, col + 1) then
                table.insert(choices, "E")
            end
            if can_walk(row, col - 1) then
                table.insert(choices, "W")
            end
            if #choices == 0 then
                return nil
            end

            local dir = choices[dungeon_random(#choices)]
            if dir == "S" then
                add_cell(row, col, "straight_down")
                rm_horiz(row, col)
                row = row + 1
                entry = "N"
            elseif dir == "E" then
                add_cell(row, col, "right_turn_from_up")
                col = col + 1
                entry = "W"
            elseif dir == "W" then
                add_cell(row, col, "left_turn_from_up")
                col = col - 1
                entry = "E"
            end

        elseif entry == "E" then
            if zone_at(row + 1, col) == "lake" then
                return nil
            end
            local choices = {}
            if can_walk(row + 1, col) then
                table.insert(choices, "S")
                table.insert(choices, "S")
                table.insert(choices, "S")
            end
            if can_walk(row, col + 1) then
                table.insert(choices, "E")
            end
            if #choices == 0 then
                return nil
            end
            local dir = choices[dungeon_random(#choices)]
            if dir == "S" then
                add_cell(row, col, "down_from_left_turn")
                row = row + 1
                entry = "N"
            elseif dir == "E" then
                add_cell(row, col, "straight_horizontal")
                col = col + 1
                entry = "E"
            end

        elseif entry == "W" then
            if zone_at(row + 1, col) == "lake" then
                return nil
            end
            local choices = {}
            if can_walk(row + 1, col) then
                table.insert(choices, "S")
                table.insert(choices, "S")
                table.insert(choices, "S")
            end
            if can_walk(row, col - 1) then
                table.insert(choices, "W")
            end
            if #choices == 0 then
                return nil
            end
            local dir = choices[dungeon_random(#choices)]
            if dir == "S" then
                add_cell(row, col, "down_from_right_turn")
                row = row + 1
                entry = "N"
            elseif dir == "W" then
                add_cell(row, col, "straight_horizontal")
                col = col - 1
                entry = "W"
            end
        end
    end

    -- exceeded max steps without reaching the lake
    return nil
end

-- ==================================
-- feature registry
-- ==================================

local feature_registry = {
    {generate = generate_river, required = true},
}

-- runs all features, returns merged overlays + filtered edges, or nil if a required feature fails
function OWFeatures.generate(zone_grid, edges, width, height)
    local overlays = {}
    local removed_set = {}

    for _, feature in ipairs(feature_registry) do
        local result = feature.generate(zone_grid, width, height)
        if not result then
            if feature.required then
                return nil
            end
        else
            for _, cell in ipairs(result.path) do
                table.insert(overlays, cell)
            end
            for _, edge in ipairs(result.removed_edges) do
                removed_set[edge.from .. "->" .. edge.to] = true
            end
        end
    end

    local filtered_edges = {}
    for _, edge in ipairs(edges) do
        if not removed_set[edge.from .. "->" .. edge.to] then
            table.insert(filtered_edges, edge)
        end
    end

    return {overlays = overlays, edges = filtered_edges}
end

return OWFeatures