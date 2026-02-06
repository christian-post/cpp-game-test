-- converts dungeon layout to WorldGraph and adds BossRoom, locked edges, item requirements

local DungeonBuilder = {}

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
        local j =dungeon_random(1, i)
        list[i], list[j] = list[j], list[i]
    end
end

local function build_graph_from_layout(WorldGraph, layout, edges)
    -- creates WorldGraph from layout and edges
    local graph = WorldGraph.new()
    
    -- add all nodes
    for name, _ in pairs(layout.positions) do
        graph:add_node(name)
    end
    
    -- add all edges
    for _, edge in ipairs(edges) do
        local from_node, to_node, requirements = edge[1], edge[2], edge[3]
        
        if #requirements == 0 then
            -- bidirectional edge already added by generator
            -- but we need to add it to graph with empty requirements
            graph:add_edge(from_node, to_node, {})
        else
            -- edge with requirements (will be added later when locking)
            graph:add_edge(from_node, to_node, requirements)
        end
    end
    
    return graph
end

local function place_boss_room(layout, edges, stairway_rooms)
    -- replaces a dead-end room on the final level with BossRoom
    -- returns the BossRoom name and updated edges
    
    local final_level = layout.levels - 1
    
    -- get all nodes on final level
    local final_level_nodes = {}
    for name, pos in pairs(layout.positions) do
        if pos.level == final_level then
            table.insert(final_level_nodes, name)
        end
    end
    
    -- count connections
    local counts = count_connections(edges, final_level_nodes)
    
    -- find dead-end rooms (excluding stairway rooms)
    local dead_end_rooms = {}
    for _, name in ipairs(final_level_nodes) do
        if counts[name] == 1 and not stairway_rooms[name] then
            table.insert(dead_end_rooms, name)
        end
    end
    
    if #dead_end_rooms == 0 then
        error("Could not find any dead-end room to place BossRoom")
    end
    
    -- pick random dead-end room
    local old_name = dead_end_rooms[dungeon_random(#dead_end_rooms)]
    local boss_room_name = "BossRoom"
    
    -- update layout
    local pos = layout.positions[old_name]
    layout.positions[old_name] = nil
    layout.positions[boss_room_name] = pos
    
    -- update edges: replace old_name with boss_room_name
    for i, edge in ipairs(edges) do
        local from_node, to_node, requirements = edge[1], edge[2], edge[3]
        
        if from_node == old_name then
            edges[i] = {boss_room_name, to_node, requirements}
        elseif to_node == old_name then
            edges[i] = {from_node, boss_room_name, requirements}
        end
    end
    
    -- find the edge connecting to BossRoom and lock it with boss_key
    for i, edge in ipairs(edges) do
        local from_node, to_node = edge[1], edge[2]
        
        if boss_room_name == from_node or boss_room_name == to_node then
            edges[i] = {from_node, to_node, {"boss_key"}}
            break
        end
    end
    
    return boss_room_name
end

local function lock_edges_with_items(edges, item_pool, stairway_rooms, boss_room_name)
    -- randomly locks edges with required items from the pool
    -- excludes edges involving Start, BossRoom, or stairway rooms
    
    -- find lockable edges (no requirements, not involving special rooms)
    local lockable_indices = {}
    for i, edge in ipairs(edges) do
        local from_node, to_node, requirements = edge[1], edge[2], edge[3]
        
        if #requirements == 0 then
            local is_special_room = from_node == "Start" or to_node == "Start" or
                                   from_node == boss_room_name or to_node == boss_room_name or
                                   stairway_rooms[from_node] or stairway_rooms[to_node]
            
            if not is_special_room then
                table.insert(lockable_indices, i)
            end
        end
    end
    
    -- shuffle item pool but exclude boss_key (already used for boss door)
    local items = {}
    for _, item in ipairs(item_pool) do
        if item ~= "boss_key" then
            table.insert(items, item)
        end
    end
    shuffle(items)
    
    -- lock random edges with items
    local num_locks = math.min(#items, #lockable_indices)
    shuffle(lockable_indices)
    
    for i = 1, num_locks do
        local edge_index = lockable_indices[i]
        local edge = edges[edge_index]
        edges[edge_index] = {edge[1], edge[2], {items[i]}}
    end
end

function DungeonBuilder.build(WorldGraph, layout, edges, stairway_rooms, item_pool)
    -- place boss room (modifies edges once)
    local boss_room_name = place_boss_room(layout, edges, stairway_rooms)
    
    -- save edge state after boss placement (deep copy)
    local edges_after_boss = {}
    for i, edge in ipairs(edges) do
        local reqs_copy = {}
        for j, req in ipairs(edge[3]) do
            reqs_copy[j] = req
        end
        edges_after_boss[i] = {edge[1], edge[2], reqs_copy}
    end
    
    local max_lock_attempts = 100
    local graph = nil
    
    for attempt = 1, max_lock_attempts do
        -- tell the Game that this is still running
        if not yield_to_engine() then
            print("INFO: Window closed during generation")
            return false
        end

        -- restore edges to post-boss state (removes previous item locks)
        for i, edge in ipairs(edges_after_boss) do
            local reqs_copy = {}
            for j, req in ipairs(edge[3]) do
                reqs_copy[j] = req
            end
            edges[i] = {edge[1], edge[2], reqs_copy}
        end
        
        -- lock edges with items (randomly)
        lock_edges_with_items(edges, item_pool, stairway_rooms, boss_room_name)
        
        -- build graph and test
        -- TODO needs to be generalized, no hard coded node names
        graph = build_graph_from_layout(WorldGraph, layout, edges)
        graph:set_start("Start")
        graph:set_goal(boss_room_name)
        
        local excluded_rooms = {}
        excluded_rooms["Start"] = true
        excluded_rooms[boss_room_name] = true
        for room_name, _ in pairs(stairway_rooms) do
            excluded_rooms[room_name] = true
        end
        
        for room_name, _ in pairs(excluded_rooms) do
            graph:exclude_room(room_name)
        end
        
        graph:initialize_items(item_pool)
        if graph:test_reachability() then
            return graph, boss_room_name, excluded_rooms
        end
        
        print(string.format("  Lock attempt %d failed reachability test, retrying...", attempt))
    end
    
    error("Could not generate solvable dungeon structure after " .. max_lock_attempts .. " attempts")
end

return DungeonBuilder