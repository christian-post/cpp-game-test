-- Analyzes completed dungeon layouts for quality metrics

local DungeonAnalyzer = {}

local function distance_between_nodes(graph, node1, node2)
    if node1 == node2 then
        return 0
    end
    
    -- build bidirectional adjacency from entire graph
    local adjacency = {}
    
    local function add_connection(from, to)
        if not adjacency[from] then
            adjacency[from] = {}
        end
        table.insert(adjacency[from], to)
    end
    
    -- iterate through all nodes and edges
    for _, node in pairs(graph.nodes) do
        for _, edge in ipairs(node.edges) do
            -- forward edge
            add_connection(node, edge.target)
            -- backward edge (for backtracking)
            add_connection(edge.target, node)
        end
    end
    
    -- BFS using bidirectional adjacency
    local visited = {[node1] = true}
    local queue = {{node = node1, distance = 0}}
    local qi = 1
    
    while qi <= #queue do
        local state = queue[qi]
        qi = qi + 1
        
        if state.node == node2 then
            return state.distance
        end
        
        if adjacency[state.node] then
            for _, neighbor in ipairs(adjacency[state.node]) do
                if not visited[neighbor] then
                    visited[neighbor] = true
                    table.insert(queue, {node = neighbor, distance = state.distance + 1})
                end
            end
        end
    end
    
    return 999  -- not connected
end

function DungeonAnalyzer.boss_key_distance(graph)
    -- find boss_key location
    local boss_key_location = nil
    for _, node in pairs(graph.nodes) do
        if node.value == "boss_key" then
            boss_key_location = node
            break
        end
    end
    
    if not boss_key_location then
        return 0
    end
    
    -- find boss room
    local boss_room = graph.nodes["BossRoom"]
    
    if not boss_room then
        return 0
    end
    
    return distance_between_nodes(graph, boss_key_location, boss_room)
end

function DungeonAnalyzer.coverage(graph)
    -- ratio of rooms with items vs total rooms
    local rooms_with_items = 0
    local total_rooms = 0
    
    for name, node in pairs(graph.nodes) do
        if not graph.excluded_rooms[name] then
            total_rooms = total_rooms + 1
            if node.value then
                rooms_with_items = rooms_with_items + 1
            end
        end
    end
    
    if total_rooms == 0 then
        return 0
    end
    
    return rooms_with_items / total_rooms
end

function DungeonAnalyzer.item_distribution(graph, item_filter)
    -- measure how spread out items are
    -- item_filter is optional function to filter which items to consider
    
    local item_locations = {}
    for _, node in pairs(graph.nodes) do
        if node.value then
            if not item_filter or item_filter(node.value) then
                table.insert(item_locations, node)
            end
        end
    end
    
    if #item_locations < 2 then
        return 1.0  -- can't measure distribution with < 2 items
    end
    
    -- calculate average pairwise distance
    local total_distance = 0
    local pairs = 0
    
    for i = 1, #item_locations do
        for j = i + 1, #item_locations do
            local dist = distance_between_nodes(graph, item_locations[i], item_locations[j])
            total_distance = total_distance + dist
            pairs = pairs + 1
        end
    end
    
    return total_distance / pairs
end

function DungeonAnalyzer.challenge_score(graph, config)
    config = config or {}
    
    -- configurable weights
    local weights = config.weights or {
        boss_distance = 0.4,
        coverage = 0.3,
        distribution = 0.3
    }
    
    -- configurable normalization
    local max_boss_distance = config.max_boss_distance or 10
    local max_avg_distribution = config.max_avg_distribution or 8
    
    -- calculate individual metrics
    local boss_dist = DungeonAnalyzer.boss_key_distance(graph)
    local coverage = DungeonAnalyzer.coverage(graph)
    local distribution = DungeonAnalyzer.item_distribution(graph, function(item)
        -- only consider keys for distribution
        return item:find("key") ~= nil
    end)
    
    -- normalize to [0, 1]
    local normalized_boss = math.min(1.0, boss_dist / max_boss_distance)
    local normalized_dist = math.min(1.0, distribution / max_avg_distribution)
    
    -- weighted combination
    local score = weights.boss_distance * normalized_boss +
                  weights.coverage * coverage +
                  weights.distribution * normalized_dist
    
    return score, {
        boss_distance = boss_dist,
        coverage = coverage,
        distribution = distribution,
        normalized_boss = normalized_boss,
        normalized_dist = normalized_dist
    }
end

function DungeonAnalyzer.generate_report(graph, config)
    local score, details = DungeonAnalyzer.challenge_score(graph, config)
    
    local report = {
        overall_score = score,
        metrics = {
            boss_key_distance = {
                value = details.boss_distance,
                normalized = details.normalized_boss,
                description = "Distance from boss key to boss room"
            },
            coverage = {
                value = details.coverage,
                normalized = details.coverage,
                description = "Ratio of rooms with items"
            },
            item_distribution = {
                value = details.distribution,
                normalized = details.normalized_dist,
                description = "Average distance between key items"
            }
        }
    }
    
    return report
end

function DungeonAnalyzer.print_report(report)
    print(string.format("\n=== Dungeon Quality Report ==="))
    print(string.format("Overall Challenge Score: %.3f\n", report.overall_score))
    
    print("Metrics:")
    for name, metric in pairs(report.metrics) do
        print(string.format("  %s: %.2f (normalized: %.3f)", 
            name, metric.value, metric.normalized))
        print(string.format("    %s", metric.description))
    end
end

return DungeonAnalyzer