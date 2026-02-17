-- this script helps with converting relative world positions to natural language

local DirectionUtils = {}

local NEARBY_THRESHOLD = 4

-- convert row,col delta to cardinal direction string
local function to_cardinal(d_row, d_col)
    local vert = ""
    local horiz = ""

    if d_row < 0 then
        vert = "north"
    elseif d_row > 0 then
        vert = "south"
    end

    if d_col < 0 then
        horiz = "west"
    elseif d_col > 0 then
        horiz = "east"
    end

    local abs_row = math.abs(d_row)
    local abs_col = math.abs(d_col)

    -- snap to dominant axis if ratio exceeds 2:1 (standard 22.5° threshold)
    if abs_row > abs_col * 2 then
        horiz = ""
    elseif abs_col > abs_row * 2 then
        vert = ""
    end

    if vert == "" and horiz == "" then
        return "nearby"
    end

    return vert .. horiz
end

-- find the entrance that matches the target level and is closest to reference room
function DirectionUtils.find_nearest_entrance(reference_room, entrances_array, target_level)
    local best = nil
    local best_dist = math.huge

    for _, entrance in ipairs(entrances_array) do
        if entrance.level == target_level then
            local dr = reference_room[1] - entrance.room[1]
            local dc = reference_room[2] - entrance.room[2]
            local dist = math.sqrt(dr * dr + dc * dc)

            if dist < best_dist then
                best_dist = dist
                best = entrance
            end
        end
    end

    return best
end

-- find the famous location closest to target, excluding the target itself
local function find_nearest_landmark(target_room, famous_locations)
    local best = nil
    local best_dist = math.huge

    for _, landmark in ipairs(famous_locations) do
        local is_target = (landmark.room[1] == target_room[1] and landmark.room[2] == target_room[2])
        if not is_target then
            local dr = target_room[1] - landmark.room[1]
            local dc = target_room[2] - landmark.room[2]
            local dist = math.sqrt(dr * dr + dc * dc)

            if dist < best_dist then
                best_dist = dist
                best = landmark
            end
        end
    end

    return best
end

-- resolve a world location to its overworld room coordinate
function DirectionUtils.resolve_to_overworld_room(world_name, room, level, dungeons_data)
    if world_name == "overworld" then
        return room
    end

    local world_data = dungeons_data[world_name]
    if not world_data then
        error("DirectionUtils: world not found in dungeons.json: " .. world_name)
    end

    if not world_data.overworld_entrances then
        error("DirectionUtils: world has no overworld_entrances: " .. world_name)
    end

    -- find entrance matching the level
    for _, entrance in ipairs(world_data.overworld_entrances) do
        if entrance.level == level then
            return entrance.room
        end
    end

    -- no exact match - use first entrance as fallback (items on other levels accessed via internal stairs)
    if #world_data.overworld_entrances > 0 then
        return world_data.overworld_entrances[1].room
    end

    error(string.format("DirectionUtils: no entrances found in world %s", world_name))
end

-- generate location description string from two overworld room coordinates
function DirectionUtils.get_direction_string(from_room, to_room, famous_locations)
    local d_row = to_room[1] - from_room[1]
    local d_col = to_room[2] - from_room[2]
    local dist  = math.sqrt(d_row * d_row + d_col * d_col)

    local dir = to_cardinal(d_row, d_col)

    if dist <= NEARBY_THRESHOLD then
        return dir .. " of here"
    else
        local landmark = find_nearest_landmark(to_room, famous_locations)
        if landmark then
            local dr = to_room[1] - landmark.room[1]
            local dc = to_room[2] - landmark.room[2]
            local landmark_dir = to_cardinal(dr, dc)
            return landmark_dir .. " of " .. landmark.display_name
        else
            -- no landmarks available, fall back to "from here"
            return dir .. " of here"
        end
    end
end

return DirectionUtils