-- generates zone layout for the overworld grid
local OverworldZoneGenerator = {}

local function create_grid(width, height, default)
    local grid = {}
    for row = 0, height - 1 do
        grid[row] = {}
        for col = 0, width - 1 do
            grid[row][col] = default
        end
    end
    return grid
end

local function can_place(grid, start_row, start_col, w, h, width, height)
    if start_row + h > height or start_col + w > width then
        return false
    end
    for row = start_row, start_row + h - 1 do
        for col = start_col, start_col + w - 1 do
            if grid[row][col] ~= "field" then
                return false
            end
        end
    end
    return true
end

local function place_rect(grid, start_row, start_col, w, h, zone_type)
    for row = start_row, start_row + h - 1 do
        for col = start_col, start_col + w - 1 do
            grid[row][col] = zone_type
        end
    end
end

local function find_valid_placements(grid, w, h, width, height, min_row)
    local placements = {}
    for row = min_row, height - h do
        for col = 0, width - w do
            if can_place(grid, row, col, w, h, width, height) then
                table.insert(placements, {row = row, col = col})
            end
        end
    end
    return placements
end

local function place_random(grid, w, h, width, height, min_row, zone_type)
    local placements = find_valid_placements(grid, w, h, width, height, min_row)
    if #placements == 0 then
        print(string.format("  Warning: could not find valid placement for zone: %s", zone_type))
        return nil
    end
    local pos = placements[dungeon_random(1, #placements)]
    place_rect(grid, pos.row, pos.col, w, h, zone_type)
    return pos
end

function OverworldZoneGenerator.generate(width, height)
    local grid = create_grid(width, height, "field")

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
    local forest_pos = place_random(grid, 3, 2, width, height, 3, "forest")
    if not forest_pos then
        return nil
    end

    -- place the lake (3x3)
    local lake_pos = place_random(grid, 3, 3, width, height, 4, "lake")
    if not lake_pos then
        return nil
    end

    -- place a town (1x2)
    --[[
    local town_pos = place_random(grid, 1, 2, width, height, 5, "town")
    if not town_pos then
        return nil
    end

    ]]

    -- place a second, smaller town (1x1)
    --[[
    local small_town_pos = place_random(grid, 1, 1, width, height, 5, "town")
    if not small_town_pos then
        return nil
    end

    ]]

    return grid
end

function OverworldZoneGenerator.print_grid(grid, width, height)
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
        print(line)
    end
end

return OverworldZoneGenerator