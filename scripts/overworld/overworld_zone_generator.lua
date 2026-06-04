-- generates zone layout for the overworld grid
local utils = require("lib.utils")
local OverworldZoneGenerator = {}

local function create_grid(width, height, default)
    -- just initializes a 2D data structure
    local grid = {}
    for row = 0, height - 1 do
        grid[row] = {}
        for col = 0, width - 1 do
            grid[row][col] = default
        end
    end
    return grid
end

local function can_place(grid, start_row, start_col, w, h, width, height, default)
    -- check if the zone is in bounds
    if start_row + h > height or start_col + w > width then
        return false
    end
    -- check if the zone contains all default tiles
    for row = start_row, start_row + h - 1 do
        for col = start_col, start_col + w - 1 do
            if grid[row][col] ~= default then
                return false
            end
        end
    end
    return true
end

local function place_rect(grid, start_row, start_col, w, h, zone_type)
    -- change the selected cells to the given zone type
    for row = start_row, start_row + h - 1 do
        for col = start_col, start_col + w - 1 do
            grid[row][col] = zone_type
        end
    end
end

local function find_valid_placements(grid, w, h, width, height, min_row, default)
    -- find all valid placements for a zone that is (w,h) in size
    local placements = {}
    for row = min_row, height - h do
        for col = 0, width - w do
            -- check if this cell is a default type cell
            if can_place(grid, row, col, w, h, width, height, default) then
                table.insert(placements, {row = row, col = col})
            end
        end
    end
    return placements
end

local function find_random_pos(grid, w, h, width, height, min_row, default)
    -- return a random valid placement (that isn't already occupied)
    local placements = find_valid_placements(grid, w, h, width, height, min_row, default)
    if #placements == 0 then
        return nil
    end
    return placements[dungeon_random(1, #placements)]
end

local function place_random(grid, w, h, width, height, min_row, zone_type, default)
    local pos = find_random_pos(grid, w, h, width, height, min_row, default)
    if not pos then
        utils.print_file(string.format("  Warning: could not find valid placement for zone: %s", zone_type))
        return nil
    end
    place_rect(grid, pos.row, pos.col, w, h, zone_type)
    return pos
end

function OverworldZoneGenerator.generate(width, height)
    local default_zone = "field"
    local grid = create_grid(width, height, default_zone)

    -- first row is always mountain
    for col = 0, width - 1 do
        grid[0][col] = "mountain"
    end

    -- in the second row there can be a mountain segment of random width
    local seg_len = dungeon_random(2, width - 2)
    local seg_start = dungeon_random(1, width - seg_len + 1) - 1
    for col = seg_start, seg_start + seg_len - 1 do
        grid[1][col] = "mountain"
    end

    -- the forest is 3x2 and gets placed randomly
    local forest_pos = place_random(grid, 3, 2, width, height, 3, "forest", default_zone)
    if not forest_pos then
        return nil
    end

    -- place the lake (2x2)
    local lake_pos = place_random(grid, 2, 2, width, height, 4, "lake", default_zone)
    if not lake_pos then
        return nil
    end

    -- place a town (1x2)

    local town_pos = place_random(grid, 1, 2, width, height, 5, "town", default_zone)
    if not town_pos then
        return nil
    end

    -- place a second, smaller town (1x1)
    -- ensure minimal distance (manhattan) between towns
    local min_dist = 4
    local dist = 0
    local small_town_pos
    local attempts = 0
    repeat
        small_town_pos = find_random_pos(grid, 1, 1, width, height, 5, default_zone)
        attempts = attempts + 1
    until (small_town_pos and utils.manhattan_dist(town_pos, small_town_pos) >= 4) or attempts >= 50

    if not small_town_pos then
        return nil
    end

    place_rect(grid, small_town_pos.row, small_town_pos.col, 1, 1, "town")

    return grid
end

function OverworldZoneGenerator.print_grid(grid, width, height)
    -- just for debugging
    local zone_chars = {
        mountain = "M",
        lake = "L",
        forest = "F",
        town = "T",
        field = "."
    }

    for row = 0, height - 1 do
        local line = "  "
        for col = 0, width - 1 do
            line = line .. (zone_chars[grid[row][col]] or "?") .. " "
        end
        utils.print_file(line)
    end
end

return OverworldZoneGenerator