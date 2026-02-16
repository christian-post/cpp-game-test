local DungeonUtils = {}

function DungeonUtils.count_table_entries(tbl)
    local count = 0
    for _ in pairs(tbl) do
        count = count + 1
    end
    return count
end

function DungeonUtils.has_skippable_item(skippable, item_name)
    for _, item_info in ipairs(skippable) do
        if item_info.item == item_name then
            return true
        end
    end
    return false
end

function DungeonUtils.is_bidirectional(graph, from_name, to_name, requirements)
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

function DungeonUtils.sort_layout_entries(layout, graph)
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

return DungeonUtils