-- generates dungeon layout using BFS room expansion with stairways between levels

local DungeonGenerator = {}

local function get_node_at(layout, level, row, col)
    -- reverse lookup: find node name at given position
    for name, pos in pairs(layout.positions) do
        if pos.level == level and pos.row == row and pos.col == col then
            return name
        end
    end
    return nil
end

local function count_connections(edges, node_names)
    -- count how many edges each node has
    local counts = {}
    for _, name in ipairs(node_names) do
        counts[name] = 0
    end
    
    for _, edge in ipairs(edges) do
        local from_node, to_node = edge[1], edge[2]
        if counts[from_node] then
            counts[from_node] = counts[from_node] + 1
        end
        if counts[to_node] then
            counts[to_node] = counts[to_node] + 1
        end
    end
    
    return counts
end

local function shuffle(list)
    -- fisher-yates shuffle
    for i = #list, 2, -1 do
        local j = dungeon_random(1, i)
        list[i], list[j] = list[j], list[i]
    end
end

local function generate_level(layout, edges, level, start_row, start_col, start_name, config)
    -- generates rooms for a single level using BFS expansion
    local rows = layout.rows
    local cols = layout.cols
    local base_dead_end_chance = config.dead_end_chance or 0.5
    local growth_rate = config.growth_rate or 4.0
    
    local visited = {}
    local queue = {}
    local room_id = 1
    
    -- place starting room
    layout.positions[start_name] = {level = level, row = start_row, col = start_col}
    visited[start_row * cols + start_col] = true
    table.insert(queue, {row = start_row, col = start_col, parent = start_name})
    
    local max_rooms = rows * cols
    
    while #queue > 0 do
        local current = table.remove(queue, 1)
        local r, c, parent_name = current.row, current.col, current.parent
        
        -- calculate dynamic dead-end chance based on fill ratio
        local fill_ratio = 0
        for _ in pairs(visited) do
            fill_ratio = fill_ratio + 1
        end
        fill_ratio = fill_ratio / max_rooms
        
        local dead_end_chance = 0
        if fill_ratio < 0.3 then
            dead_end_chance = 0.0
        else
            local excess = (fill_ratio - 0.3) / 0.7
            dead_end_chance = base_dead_end_chance * (growth_rate ^ excess - 1) / (growth_rate - 1)
        end
        
        if dungeon_random() < dead_end_chance then
            goto continue
        end
        
        -- try to expand in random directions
        local directions = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}}
        local max_expands = dungeon_random(1, #directions)
        shuffle(directions)
        
        local expand_count = 0
        for _, dir in ipairs(directions) do
            if expand_count >= max_expands then
                break
            end
            
            local nr, nc = r + dir[1], c + dir[2]
            local cell_key = nr * cols + nc
            
            if nr >= 0 and nr < rows and nc >= 0 and nc < cols and not visited[cell_key] then
                local room_name = string.format("L%d_Room%d", level, room_id)
                room_id = room_id + 1
                
                layout.positions[room_name] = {level = level, row = nr, col = nc}
                visited[cell_key] = true
                table.insert(queue, {row = nr, col = nc, parent = room_name})
                
                -- add bidirectional edge (no requirements yet)
                table.insert(edges, {parent_name, room_name, {}})
                
                expand_count = expand_count + 1
            end
        end
        
        ::continue::
    end
end

local function find_dead_end_for_stairs(layout, edges, level, entry_position)
    -- finds a dead-end room suitable for stairway placement
    -- excludes the starting room and optionally the entry stairway position
    
    -- get all nodes on this level
    local level_nodes = {}
    for name, pos in pairs(layout.positions) do
        if pos.level == level then
            table.insert(level_nodes, name)
        end
    end

    -- sort for deterministic order
    table.sort(level_nodes)
    
    -- count connections for each node
    local counts = count_connections(edges, level_nodes)
    
    -- find dead-end candidates (connection count = 1)
    local candidates = {}
    for _, name in ipairs(level_nodes) do
        local is_start = (level == 0 and name == "Start") or (level > 0 and name == string.format("L%d_Start", level))
        
        if not is_start and counts[name] == 1 then
            -- check if this is not the entry position (for intermediate levels)
            if entry_position then
                local pos = layout.positions[name]
                if pos.row ~= entry_position.row or pos.col ~= entry_position.col then
                    table.insert(candidates, name)
                end
            else
                table.insert(candidates, name)
            end
        end
    end
    
    if #candidates == 0 then
        return nil
    end
    
    return candidates[dungeon_random(#candidates)]
end

function DungeonGenerator.generate(config)
    -- generates complete dungeon layout with multiple levels
    -- config: {rows, cols, levels, start_row, start_col, dead_end_chance, growth_rate}
    
    local rows = config.rows or 2
    local cols = config.cols or 3
    local levels = config.levels or 1
    local start_row = config.start_row or 0
    local start_col = config.start_col or 0
    
    local layout = {
        rows = rows,
        cols = cols,
        levels = levels,
        positions = {}
    }
    
    local edges = {}
    local stairway_positions = {}  -- stores {row, col} for each level connection
    local stairway_rooms = {}  -- set of room names that have stairways
    
    -- generate each level
    for level = 0, levels - 1 do
        local level_start_row, level_start_col, level_start_name
        
        if level == 0 then
            level_start_row = start_row
            level_start_col = start_col
            level_start_name = "Start"
        else
            -- start at same position as stairway from previous level
            local stair_pos = stairway_positions[level]
            level_start_row = stair_pos.row
            level_start_col = stair_pos.col
            level_start_name = string.format("L%d_Room0", level)
        end
        
        generate_level(layout, edges, level, level_start_row, level_start_col, level_start_name, config)
        
        -- find stairway location for next level (except on last level)
        if level < levels - 1 then
            local entry_position = nil
            if level > 0 then
                entry_position = stairway_positions[level]
            end
            
            local stairs_room = find_dead_end_for_stairs(layout, edges, level, entry_position)
            
            if not stairs_room then
                error(string.format("Could not find dead-end room for stairs on level %d", level))
            end
            
            local stairs_pos = layout.positions[stairs_room]
            table.insert(stairway_positions, {row = stairs_pos.row, col = stairs_pos.col})
        end
    end
    
    -- create bidirectional edges between levels at stairway positions
    for level = 0, levels - 2 do
        local stair_pos = stairway_positions[level + 1]
        
        local stairs_from = get_node_at(layout, level, stair_pos.row, stair_pos.col)
        local stairs_to = get_node_at(layout, level + 1, stair_pos.row, stair_pos.col)
        
        if stairs_from and stairs_to then
            table.insert(edges, {stairs_from, stairs_to, {}})
            table.insert(edges, {stairs_to, stairs_from, {}})
            stairway_rooms[stairs_from] = true
            stairway_rooms[stairs_to] = true
        end
    end
    
    return layout, edges, stairway_rooms
end

function DungeonGenerator.calculate_doors(layout, edges, level, row, col)
    -- calculates door pattern for a room based on edges
    -- returns binary represenation as string: "RULD" (right, up, left, down) with 1=door, 0=wall
    
    local node_name = get_node_at(layout, level, row, col)
    if not node_name then
        return "0000"
    end
    
    local doors = {0, 0, 0, 0}  -- [1]=right, [2]=up, [3]=left, [4]=down
    
    -- check all edges for connections to adjacent rooms
    for _, edge in ipairs(edges) do
        local from_node, to_node = edge[1], edge[2]
        
        -- check if this edge involves our room
        if from_node == node_name or to_node == node_name then
            local other_node = (from_node == node_name) and to_node or from_node
            local other_pos = layout.positions[other_node]
            
            if other_pos and other_pos.level == level then
                local dr = other_pos.row - row
                local dc = other_pos.col - col
                
                if dr == 0 and dc == 1 then
                    doors[1] = 1  -- right
                elseif dr == -1 and dc == 0 then
                    doors[2] = 1  -- up
                elseif dr == 0 and dc == -1 then
                    doors[3] = 1  -- left
                elseif dr == 1 and dc == 0 then
                    doors[4] = 1  -- down
                end
            end
        end
    end
    
    return string.format("%d%d%d%d", doors[1], doors[2], doors[3], doors[4])
end


return DungeonGenerator