-- utility function specific to World generation
local M = {}

function M.get_node_at(layout, level, row, col)
    -- reverse lookup: find node name at given position
    for name, pos in pairs(layout.positions) do
        if pos.level == level and pos.row == row and pos.col == col then
            return name
        end
    end
    return nil
end

function M.count_table_entries(tbl)
    local count = 0
    for _ in pairs(tbl) do
        count = count + 1
    end
    return count
end

function M.has_skippable_item(skippable, item_name)
    for _, item_info in ipairs(skippable) do
        if item_info.item == item_name then
            return true
        end
    end
    return false
end

function M.is_bidirectional(graph, from_name, to_name, requirements)
    local from_node = graph.nodes[from_name]
    local to_node = graph.nodes[to_name]

    -- check if reverse edge exists with same requirements
    for _, edge in ipairs(to_node.edges) do
        if edge.target == from_node then
            -- check if requirements match
            if #edge.requirements == #requirements then
                local match = true
                for i = 1, #requirements do
                    if edge.requirements[i] ~= requirements[i] then
                        match = false
                        break
                    end
                end
                if match then
                    return true
                end
            end
        end
    end
    return false
end

function M.sort_layout_entries(layout, graph)
    local entries = {}
    for name, pos in pairs(layout.positions) do
        local item = graph.nodes[name].value or "empty"
        table.insert(entries, {name = name, pos = pos, item = item})
    end

    -- sort by level, then row, then col
    table.sort(entries, function(a, b)
        if a.pos.level ~= b.pos.level then
            return a.pos.level < b.pos.level
        end
        if a.pos.row ~= b.pos.row then
            return a.pos.row < b.pos.row
        end
        return a.pos.col < b.pos.col
    end)

    return entries
end

function M.count_connections(edges, node_names)
    -- count how many edges each node has
    local counts = {}
    for _, name in ipairs(node_names) do
        counts[name] = 0
    end
    
    for _, edge in ipairs(edges) do
        if counts[edge.from] then
            counts[edge.from] = counts[edge.from] + 1
        end
        if counts[edge.to] then
            counts[edge.to] = counts[edge.to] + 1
        end
    end
    
    return counts
end

function M.node_name(row, col)
    -- constructs a unique key from a row,col pair
    return string.format("OW_%d_%d", row, col)
end

return M