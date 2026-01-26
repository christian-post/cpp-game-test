-- Dungeon graph

-- ============================================================================
-- Item Configuration
-- ============================================================================
local ItemConfig = {
    consumable_items = {
        small_key = true,
        boss_key = false,  -- boss key is permanent
    }
}

local function is_consumable(item_name)
    return ItemConfig.consumable_items[item_name] == true
end

-- ============================================================================
-- Inventory Management
-- ============================================================================
local Inventory = {}

function Inventory.new()
    return {}
end

function Inventory.copy(inv)
    local new_inv = {}
    for item, count in pairs(inv) do
        new_inv[item] = count
    end
    return new_inv
end

function Inventory.add(inv, item)
    if is_consumable(item) then
        inv[item] = (inv[item] or 0) + 1
    else
        inv[item] = true
    end
end

function Inventory.has(inv, item, count)
    count = count or 1
    if is_consumable(item) then
        return (inv[item] or 0) >= count
    else
        return inv[item] == true
    end
end

function Inventory.consume(inv, item)
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
    -- create a string key for state tracking
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

-- ============================================================================
-- Node
-- ============================================================================
local Node = {}
Node.__index = Node

function Node.new(name)
    return setmetatable({
        name = name,
        edges = {},
        value = nil
    }, Node)
end

-- ============================================================================
-- Edge
-- ============================================================================
local Edge = {}
Edge.__index = Edge

function Edge.new(target, requirements)
    return setmetatable({
        target = target,
        requirements = requirements or {}
    }, Edge)
end

function Edge:can_traverse(inventory)
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

-- ============================================================================
-- WorldGraph
-- ============================================================================
local WorldGraph = {}
WorldGraph.__index = WorldGraph

function WorldGraph.new()
    return setmetatable({
        nodes = {},
        start = nil,
        owned_items = Inventory.new(),
        item_pool = {},
        excluded_rooms = {},
        collected_locations = {}
    }, WorldGraph)
end

function WorldGraph:exclude_room(room_name)
    self.excluded_rooms[room_name] = true
end

function WorldGraph:add_node(name)
    local node = Node.new(name)
    self.nodes[name] = node
    return node
end

function WorldGraph:add_edge(from_name, to_name, requirements)
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
    self.start = self.nodes[name]
end

function WorldGraph:initialize_items(item_pool)
    -- copy and shuffle
    self.item_pool = {}
    for i = 1, #item_pool do
        self.item_pool[i] = item_pool[i]
    end
    
    -- fisher-yates shuffle
    for i = #self.item_pool, 2, -1 do
        local j = math.random(1, i)
        self.item_pool[i], self.item_pool[j] = self.item_pool[j], self.item_pool[i]
    end
    
    self.owned_items = Inventory.new()
end

function WorldGraph:get_reachable_nodes()
    if not self.start then
        return {}
    end
    
    local starting_inventory = Inventory.copy(self.owned_items)
    
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
    
    local visited = {}
    local visited_nodes = {}  -- Track which nodes already added to reachable
    local reachable = {}
    local queue = {{node = self.start, inventory = starting_inventory}}
    local qi = 1
    
    while qi <= #queue do
        local state = queue[qi]
        qi = qi + 1
        
        local key = state_key(state.node, state.inventory)
        
        if not visited[key] then
            visited[key] = true
            
            -- Only add node once to reachable (not once per inventory state!)
            if not visited_nodes[state.node] then
                visited_nodes[state.node] = true
                table.insert(reachable, state.node)
            end
            
            for _, edge in ipairs(state.node.edges) do
                if edge:can_traverse(state.inventory) then
                    local new_inventory = edge:traverse(state.inventory)
                    table.insert(queue, {node = edge.target, inventory = new_inventory})
                end
            end
        end
    end
    
    return reachable
end

function WorldGraph:forward_fill(verbose)
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
        local node = null_nodes[math.random(#null_nodes)]
        local item = table.remove(self.item_pool)
        node.value = item
        Inventory.add(self.owned_items, item)
        self.collected_locations[node.name] = true  -- Mark as collected
        
        if verbose then
            print(string.format("Placed '%s' in '%s'", item, node.name))
        end
        
        self:search()
    end
    
    return true
end

function WorldGraph:reset_items()
    for _, node in pairs(self.nodes) do
        node.value = nil
    end
    
    self.owned_items = Inventory.new()
    self.item_pool = {}
    self.collected_locations = {}
end

function WorldGraph:has_null_node()
    for _, node in pairs(self.nodes) do
        if not node.value then
            return true
        end
    end
    return false
end

function WorldGraph:search()
    -- Track which locations we've already counted
    if not self.collected_locations then
        self.collected_locations = {}
    end
    
    while true do
        if not self.start then
            return
        end
        
        local reachable = self:get_reachable_nodes()
        
        -- Collect items from reachable nodes
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
        
        -- Update inventory
        local acquired_any = false
        for item in pairs(newly_acquired) do
            Inventory.add(self.owned_items, item)
            acquired_any = true
        end
        
        -- Mark locations as collected
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
        
        -- Show which nodes are unreachable
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
-- Export
-- ============================================================================
return {
    new = WorldGraph.new,
    configure = function(config)
        if config.consumable_items then
            ItemConfig.consumable_items = config.consumable_items
        end
    end
}