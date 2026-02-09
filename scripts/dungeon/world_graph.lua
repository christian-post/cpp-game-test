-- Item configuration
--
-- defines which items are consumable (stackable) vs permanent (boolean flags).
-- consumable items are removed from inventory when used to traverse edges,
-- while permanent items remain forever once acquired.
local ItemConfig = {
    consumable_items = {
        key = true,
        boss_key = false,  -- boss key is permanent
    }
}

local function is_consumable(item_name)
    return ItemConfig.consumable_items[item_name] == true
end

-- Inventory management
--
-- manages item storage with support for both consumable (stackable) and 
-- permanent items. consumables track quantities, permanent items are boolean flags.
-- inventories are immutable: operations return new copies rather than modifying in place.

local Inventory = {}

function Inventory.new()
    -- creates an empty inventory
    return {}
end

function Inventory.copy(inv)
    -- creates a deep copy of inventory
    local new_inv = {}
    for item, count in pairs(inv) do
        new_inv[item] = count
    end
    return new_inv
end

function Inventory.add(inv, item)
    -- adds this item to inventory (mutates in place)
    if is_consumable(item) then
        inv[item] = (inv[item] or 0) + 1
    else
        inv[item] = true
    end
end

function Inventory.has(inv, item, count)
    -- check if inventory contains item(s)
    count = count or 1
    if is_consumable(item) then
        return (inv[item] or 0) >= count
    else
        return inv[item] == true
    end
end

function Inventory.consume(inv, item)
    -- create new inventory with one item consumed
    local new_inv = Inventory.copy(inv)
    if is_consumable(item) then
        new_inv[item] = (new_inv[item] or 0) - 1
        if new_inv[item] <= 0 then
            new_inv[item] = nil
        end
    end
    -- permanent items are never consumed
    return new_inv
end

function Inventory.to_key(inv)
    -- creates a string key for state tracking in BFS
    local items = {}
    for item, value in pairs(inv) do
        if type(value) == "number" then
            table.insert(items, item .. ":" .. value)
        else
            table.insert(items, item)
        end
    end
    table.sort(items)
    return table.concat(items, ",")
end

-- Node
--
-- represents a room in the dungeon graph. each node has a name (room identifier),
-- a list of edges to connected rooms, and an optional item value placed in this room.
local Node = {}
Node.__index = Node

function Node.new(name)
    return setmetatable({
        name = name, -- unique room identifier
        edges = {}, -- array of Edge objects connecting to other nodes
        value = nil -- item placed in this room (nil if empty)
    }, Node)
end

-- Edge
--
-- represents a connection between two rooms with optional item requirements.
-- edges can be traversed if the player's inventory contains all required items.
-- consumable items are removed from inventory when traversing.
local Edge = {}
Edge.__index = Edge

function Edge.new(target, requirements)
    return setmetatable({
        target = target, -- destination Node
        requirements = requirements or {} -- array of item names needed to traverse this edge
    }, Edge)
end

function Edge:can_traverse(inventory)
    -- check if edge is traversable with given inventory
    for _, req in ipairs(self.requirements) do
        if not Inventory.has(inventory, req) then
            return false
        end
    end
    return true
end

function Edge:traverse(inventory)
    -- returns new inventory after consuming required items
    local new_inv = Inventory.copy(inventory)
    for _, req in ipairs(self.requirements) do
        if is_consumable(req) then
            new_inv = Inventory.consume(new_inv, req)
        end
    end
    return new_inv
end

-- world graph
--
-- represents a dungeon as a directed graph with items and traversal requirements.
-- supports forward-fill item placement algorithm with reachability checking.
-- uses BFS to determine which rooms are accessible given current inventory.
local WorldGraph = {}
WorldGraph.__index = WorldGraph

function WorldGraph.new()
    -- creates a new empty world graph
    return setmetatable({
        nodes = {},
        node_to_name = {}, -- reverse lookup (to prevent unintentional shuffling)
        start = nil, -- start node
        goal = nil, -- final node for world completion
        owned_items = Inventory.new(), -- contains items that have been collected during search(), empty at the start
        item_pool = {}, -- items that are to be placed, full after initialization
        excluded_rooms = {},
        collected_locations = {}
    }, WorldGraph)
end

function WorldGraph:exclude_room(room_name)
    -- marks room as ineligible for item placement
    self.excluded_rooms[room_name] = true
end

function WorldGraph:add_node(name)
    -- adds a room to the graph
    local node = Node.new(name)
    self.nodes[name] = node
    self.node_to_name[node] = name
    return node
end

function WorldGraph:set_goal(name)
    self.goal = self.nodes[name]
end

function WorldGraph:add_edge(from_name, to_name, requirements)
    -- adds bidirectional connection between rooms with optional requirements
    -- the nodes have to already exist in the graph
    local from_node = self.nodes[from_name]
    local to_node = self.nodes[to_name]
    
    if not from_node or not to_node then
        error(string.format("Node not found: %s or %s", from_name, to_name))
    end
    
    requirements = requirements or {}
    
    -- add forward edge
    local forward_edge = Edge.new(to_node, requirements)
    table.insert(from_node.edges, forward_edge)
    
    -- add reverse edge (bidirectional by default)
    local reverse_edge = Edge.new(from_node, requirements)
    table.insert(to_node.edges, reverse_edge)
end

function WorldGraph:add_one_way_edge(from_name, to_name, requirements)
    -- adds unidirectional connection between rooms with optional requirements
    local from_node = self.nodes[from_name]
    local to_node = self.nodes[to_name]
    
    if not from_node or not to_node then
        error(string.format("Node not found: %s or %s", from_name, to_name))
    end
    
    requirements = requirements or {}
    local edge = Edge.new(to_node, requirements)
    table.insert(from_node.edges, edge)
end

function WorldGraph:set_start(name)
    -- sets the starting room
    self.start = self.nodes[name]
end

function WorldGraph:initialize_items(item_pool)
    -- loads and shuffles item pool for placement

    -- copy and shuffle
    self.item_pool = {}
    for i = 1, #item_pool do
        self.item_pool[i] = item_pool[i]
    end
    
    -- fisher-yates shuffle
    for i = #self.item_pool, 2, -1 do
        local j = dungeon_random(1, i)
        self.item_pool[i], self.item_pool[j] = self.item_pool[j], self.item_pool[i]
    end
    
    self.owned_items = Inventory.new()
end

function WorldGraph:get_reachable_nodes()
    -- uses BFS to find all accessible rooms from start with current inventory
    if not self.start then
        return {}
    end
    
    local starting_inventory = Inventory.copy(self.owned_items)
    
    -- helper function to track visited nodes
    -- state = (node, inventory)
    local state_key = function(node, inv)
        local node_name = ""
        for name, n in pairs(self.nodes) do
            if n == node then
                node_name = name
                break
            end
        end
        return node_name .. "|" .. Inventory.to_key(inv)
    end
    
    local visited_states = {}  -- tracks (node, inventory) combinations already explored
    local visited_nodes = {}  -- track which unique nodes were already added to reachable array
    local reachable = {}  -- nodes that can be reached, returned at the end
    local queue = {{node = self.start, inventory = starting_inventory}}
    local qi = 1
    
    while qi <= #queue do
        local state = queue[qi]
        qi = qi + 1
        
        local key = state_key(state.node, state.inventory)
        
        if not visited_states[key] then
            visited_states[key] = true
            
            -- add node once to reachable nodes
            if not visited_nodes[state.node] then
                visited_nodes[state.node] = true
                table.insert(reachable, state.node)
            end
            
            for _, edge in ipairs(state.node.edges) do
                if edge:can_traverse(state.inventory) then
                    local new_inventory = edge:traverse(state.inventory)
                    -- add the node at the other end of the edge to the queue, to be checked in the next iteration
                    table.insert(queue, {node = edge.target, inventory = new_inventory})
                end
            end
        end
    end
    
    return reachable
end

function WorldGraph:forward_fill(verbose)
    -- places items using forward-fill algorithm
    local iteration = 0
    
    while self:has_null_node() and #self.item_pool > 0 do
        iteration = iteration + 1
        
        if verbose then
            print(string.format("\n=== Forward Fill Iteration %d ===", iteration))
            print("Current inventory:")
            for item, value in pairs(self.owned_items) do
                if type(value) == "number" then
                    print(string.format("  %s x%d", item, value))
                else
                    print(string.format("  %s", item))
                end
            end
            print(string.format("Items left to place: %d", #self.item_pool))
        end
        
        local reachable = self:get_reachable_nodes()
        
        if verbose then
            print("Reachable nodes:")
            for _, node in ipairs(reachable) do
                local val = node.value or "empty"
                print(string.format("  %s (%s)", node.name, val))
            end
        end
        
        local null_nodes = {}
        
        for _, node in ipairs(reachable) do
            if not node.value and not self.excluded_rooms[node.name] then
                -- this node is allowed to have an item
                table.insert(null_nodes, node)
            end
        end
        
        if verbose then
            print("Available nodes for placement:")
            for _, node in ipairs(null_nodes) do
                print(string.format("  %s", node.name))
            end
        end
        
        if #null_nodes == 0 then
            if verbose then
                print("Warning: no reachable node available for item placement.")
            end
            return false
        end
        
        -- place random item in random null node
        local node = null_nodes[dungeon_random(#null_nodes)]
        local item = table.remove(self.item_pool)
        node.value = item
        Inventory.add(self.owned_items, item)
        self.collected_locations[node.name] = true  -- mark as collected
        
        --if verbose then
            print(string.format("Placed '%s' in '%s'", item, node.name))
        --end
        
        -- expand inventory by going through all reachable nodes
        self:search()
    end
    
    return true
end


function WorldGraph:reset_items()
    -- clears all item placements and resets state
    for _, node in pairs(self.nodes) do
        node.value = nil
    end
    
    self.owned_items = Inventory.new()
    self.item_pool = {}
    self.collected_locations = {}
end

function WorldGraph:has_null_node()
    -- checks if any node is missing an item value
    for _, node in pairs(self.nodes) do
        if not node.value then
            return true
        end
    end
    return false
end

function WorldGraph:search()
    -- collects items from reachable rooms and updates inventory
    -- track which locations we've already counted
    if not self.collected_locations then
        self.collected_locations = {}
    end
    
    while true do
        if not self.start then
            return
        end
        
        local reachable = self:get_reachable_nodes()
        
        -- collect items from reachable nodes
        local newly_acquired = {}
        for _, node in ipairs(reachable) do
            if node.value and not self.collected_locations[node.name] then
                if is_consumable(node.value) then
                    newly_acquired[node.value] = true
                else
                    if not self.owned_items[node.value] then
                        newly_acquired[node.value] = true
                    end
                end
            end
        end
        
        -- update inventory
        local acquired_any = false
        for item in pairs(newly_acquired) do
            Inventory.add(self.owned_items, item)
            acquired_any = true
        end
        
        -- mark locations as collected
        for _, node in ipairs(reachable) do
            if node.value then
                self.collected_locations[node.name] = true
            end
        end
        
        if not acquired_any then
            break
        end
    end
end

function WorldGraph:test_reachability()
    -- verifies all nodes can be reached with all items in the pool
    -- save current owned items
    local saved_items = self.owned_items
    
    -- create inventory with all items from pool
    self.owned_items = Inventory.new()
    for _, item in ipairs(self.item_pool) do
        Inventory.add(self.owned_items, item)
    end
    
    -- get reachable nodes with all items
    local reachable = self:get_reachable_nodes()
    
    -- restore owned items
    self.owned_items = saved_items
    
    -- build set of reachable node names
    local reachable_names = {}
    for _, node in ipairs(reachable) do
        for name, n in pairs(self.nodes) do
            if n == node then
                reachable_names[name] = true
                break
            end
        end
    end
    
    -- count total nodes
    local total_nodes = 0
    for _ in pairs(self.nodes) do
        total_nodes = total_nodes + 1
    end
    
    local num_reachable = #reachable
    
    if num_reachable < total_nodes then
        local diff = total_nodes - num_reachable
        print(string.format("WARNING: There are %d unreachable nodes (out of %d).", diff, total_nodes))
        
        -- show which nodes are unreachable
        print("\nUnreachable nodes:")
        for name, _ in pairs(self.nodes) do
            if not reachable_names[name] then
                print("  " .. name)
            end
        end
        
        return false
    else
        print(string.format("INFO: All %d nodes can be reached", total_nodes))
        return true
    end
end

-- ============================================================================
-- SOLVABILITY CHECKING
-- ============================================================================

-- create unique identifier for a door (bidirectional)
local function door_id(node1_name, node2_name)
    -- normalize: alphabetically smaller name first for consistency
    if node1_name < node2_name then
        return node1_name .. "<->" .. node2_name
    else
        return node2_name .. "<->" .. node1_name
    end
end

-- state representation for solvability checking
local function state_to_key(graph, node, inventory, collected, unlocked_doors)
    local node_name = graph.node_to_name[node]
    
    local collected_list = {}
    for loc, _ in pairs(collected) do
        table.insert(collected_list, loc)
    end
    table.sort(collected_list)
    
    local unlocked_list = {}
    for door, _ in pairs(unlocked_doors) do
        table.insert(unlocked_list, door)
    end
    table.sort(unlocked_list)
    
    return node_name .. "|" .. Inventory.to_key(inventory) .. "|" .. 
           table.concat(collected_list, ",") .. "|" .. 
           table.concat(unlocked_list, ",")
end

function WorldGraph:find_all_reachable_states()
    -- finds ALL states (node + inventory + collected + unlocked doors) reachable from start
    if not self.start then
        return {}
    end
    
    local start_state = {
        node = self.start,
        inventory = Inventory.new(),
        collected = {},
        unlocked_doors = {}
    }
    
    local queue = {start_state}
    local visited = {}
    local all_states = {start_state}
    visited[state_to_key(self, start_state.node, start_state.inventory, start_state.collected, start_state.unlocked_doors)] = true
    
    local qi = 1
    while qi <= #queue do
        local current = queue[qi]
        qi = qi + 1
        
        -- try to collect item at current location if not already collected
        if current.node.value and not current.collected[current.node.name] then
            local new_inventory = Inventory.copy(current.inventory)
            Inventory.add(new_inventory, current.node.value)
            
            local new_collected = {}
            for k, v in pairs(current.collected) do
                new_collected[k] = v
            end
            new_collected[current.node.name] = true
            
            local new_state = {
                node = current.node,
                inventory = new_inventory,
                collected = new_collected,
                unlocked_doors = current.unlocked_doors
            }
            
            local key = state_to_key(self, new_state.node, new_state.inventory, new_state.collected, new_state.unlocked_doors)
            if not visited[key] then
                visited[key] = true
                table.insert(queue, new_state)
                table.insert(all_states, new_state)
            end
        end
        
        -- try to traverse each edge
        for _, edge in ipairs(current.node.edges) do
            local current_node_name = self.node_to_name[current.node]
            local target_node_name = self.node_to_name[edge.target]
            local edge_door_id = door_id(current_node_name, target_node_name)
            
            -- check if this is a free edge or already unlocked
            if #edge.requirements == 0 or current.unlocked_doors[edge_door_id] then
                local new_state = {
                    node = edge.target,
                    inventory = current.inventory,
                    collected = current.collected,
                    unlocked_doors = current.unlocked_doors
                }
                
                local key = state_to_key(self, new_state.node, new_state.inventory, new_state.collected, new_state.unlocked_doors)
                if not visited[key] then
                    visited[key] = true
                    table.insert(queue, new_state)
                    table.insert(all_states, new_state)
                end
            -- check if we can unlock this door
            elseif edge:can_traverse(current.inventory) then
                local new_inventory = edge:traverse(current.inventory)
                
                -- copy unlocked doors and add this one
                local new_unlocked = {}
                for k, v in pairs(current.unlocked_doors) do
                    new_unlocked[k] = v
                end
                new_unlocked[edge_door_id] = true
                
                local new_state = {
                    node = edge.target,
                    inventory = new_inventory,
                    collected = current.collected,
                    unlocked_doors = new_unlocked
                }
                
                local key = state_to_key(self, new_state.node, new_state.inventory, new_state.collected, new_state.unlocked_doors)
                if not visited[key] then
                    visited[key] = true
                    table.insert(queue, new_state)
                    table.insert(all_states, new_state)
                end
            end
        end
    end
    
    return all_states
end

function WorldGraph:find_skippable_items()
    -- returns items that are not required to reach the goal
    if not self.goal then
        error("Goal node not set! Call set_goal() first")
    end
    
    local all_states = self:find_all_reachable_states()
    
    -- find all item locations (excluding excluded rooms)
    local item_locations = {}
    for name, node in pairs(self.nodes) do
        if node.value and not self.excluded_rooms[name] then
            item_locations[name] = node.value
        end
    end
    
    -- find items that were MISSED in at least one state AT THE GOAL
    local skippable = {}
    for loc, item in pairs(item_locations) do
        skippable[loc] = {location = loc, item = item, can_skip = false}
    end
    
    for _, state in ipairs(all_states) do
        if state.node == self.goal then
            -- this state reached the goal - check which items it DIDN'T collect
            for loc, _ in pairs(item_locations) do
                if not state.collected[loc] then
                    skippable[loc].can_skip = true
                end
            end
        end
    end
    
    -- return only skippable items
    local result = {}
    for _, info in pairs(skippable) do
        if info.can_skip then
            table.insert(result, {location = info.location, item = info.item})
        end
    end
    
    return result
end

function WorldGraph:can_reach_goal_from_state(start_state)
    -- checks if a specific state can reach the goal node
    if not self.goal then
        error("Goal node not set! Call set_goal() first")
    end
    
    local queue = {start_state}
    local visited = {}
    visited[state_to_key(self, start_state.node, start_state.inventory, start_state.collected, start_state.unlocked_doors)] = true
    
    local qi = 1
    while qi <= #queue do
        local current = queue[qi]
        qi = qi + 1
        
        -- check if we reached goal
        if current.node == self.goal then
            return true
        end
        
        -- try to collect item
        if current.node.value and not current.collected[current.node.name] then
            local new_inventory = Inventory.copy(current.inventory)
            Inventory.add(new_inventory, current.node.value)
            
            local new_collected = {}
            for k, v in pairs(current.collected) do
                new_collected[k] = v
            end
            new_collected[current.node.name] = true
            
            local new_state = {
                node = current.node,
                inventory = new_inventory,
                collected = new_collected,
                unlocked_doors = current.unlocked_doors
            }
            
            local key = state_to_key(self, new_state.node, new_state.inventory, new_state.collected, new_state.unlocked_doors)
            if not visited[key] then
                visited[key] = true
                table.insert(queue, new_state)
            end
        end
        
        -- try to traverse edges
        for _, edge in ipairs(current.node.edges) do
            local current_node_name = self.node_to_name[current.node]
            local target_node_name = self.node_to_name[edge.target]
            local edge_door_id = door_id(current_node_name, target_node_name)
            
            -- check if this is a free edge or already unlocked
            if #edge.requirements == 0 or current.unlocked_doors[edge_door_id] then
                local new_state = {
                    node = edge.target,
                    inventory = current.inventory,
                    collected = current.collected,
                    unlocked_doors = current.unlocked_doors
                }
                
                local key = state_to_key(self, new_state.node, new_state.inventory, new_state.collected, new_state.unlocked_doors)
                if not visited[key] then
                    visited[key] = true
                    table.insert(queue, new_state)
                end
            -- check if we can unlock this door
            elseif edge:can_traverse(current.inventory) then
                local new_inventory = edge:traverse(current.inventory)
                
                -- copy unlocked doors and add this one
                local new_unlocked = {}
                for k, v in pairs(current.unlocked_doors) do
                    new_unlocked[k] = v
                end
                new_unlocked[edge_door_id] = true
                
                local new_state = {
                    node = edge.target,
                    inventory = new_inventory,
                    collected = current.collected,
                    unlocked_doors = new_unlocked
                }
                
                local key = state_to_key(self, new_state.node, new_state.inventory, new_state.collected, new_state.unlocked_doors)
                if not visited[key] then
                    visited[key] = true
                    table.insert(queue, new_state)
                end
            end
        end
    end
    
    return false
end

function WorldGraph:is_solvable(verbose)
    -- checks if dungeon is solvable (no reachable state leads to dead end)
    if not self.goal then
        error("Goal node not set! Call set_goal() first")
    end
    
    if verbose then
        print("  Checking for trap states...")
    end
    
    -- find all reachable states
    local all_states = self:find_all_reachable_states()
    
    if verbose then
        print(string.format("  Found %d reachable states to check", #all_states))
    end
    
    -- check each state can reach goal
    for i, state in ipairs(all_states) do
        if not self:can_reach_goal_from_state(state) then
            if verbose then
                print(string.format("  TRAP FOUND at state %d (location: %s)", i, state.node.name))
            end
            return false, state
        end
    end
    
    if verbose then
        print("  No traps found!")
    end
    
    return true, nil
end

-- export
return {
    new = WorldGraph.new,
    configure = function(config)
        if config.consumable_items then
            ItemConfig.consumable_items = config.consumable_items
        end
    end
}