-- scripts that help debugging the dungeon generation process
local DebugHelper = {}

function DebugHelper.print_generation_summary(layout, edges)
    local room_count = 0
    for _ in pairs(layout.positions) do
        room_count = room_count + 1
    end
    
    print(string.format("  Generated %d rooms across %d levels", room_count, layout.levels))
    print(string.format("  Created %d edges", #edges))
end

function DebugHelper.print_layout(layout, graph)
    print("\nLayout:")
    
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

    -- print sorted entries
    for _, entry in ipairs(entries) do
        print(string.format("  L%d [%d,%d] %s: %s", entry.pos.level, entry.pos.row, entry.pos.col, entry.name, entry.item))
    end
end

function DebugHelper.print_door_patterns(layout, edges, DungeonGenerator)
    print("\nDoor patterns:")
    for level = 0, layout.levels - 1 do
        print(string.format("  Level %d:", level))
        for row = 0, layout.rows - 1 do
            for col = 0, layout.cols - 1 do
                local node_name = nil
                for name, pos in pairs(layout.positions) do
                    if pos.level == level and pos.row == row and pos.col == col then
                        node_name = name
                        break
                    end
                end
                    
                if node_name then
                    local doors = DungeonGenerator.calculate_doors(layout, edges, level, row, col)
                    print(string.format("    [%d,%d] %s: %s", row, col, node_name, doors))
                end
            end
        end
    end
end

function DebugHelper.print_edges(layout, edges, graph)
    print("\nEdges:")
    
    -- collect edges into a table for sorting
    local edge_entries = {}
    for _, edge in ipairs(edges) do
        local from_pos = layout.positions[edge.from]
        local to_pos = layout.positions[edge.to]
    
        -- skip edges where positions aren't found
        if from_pos and to_pos then
            local locked_item = nil
            if edge.requirements and #edge.requirements > 0 then
                locked_item = edge.requirements[1]
            end
            table.insert(edge_entries, {
                from = edge.from,
                to = edge.to,
                from_pos = from_pos,
                to_pos = to_pos,
                locked_item = locked_item,
                requirements = edge.requirements or {}
            })
        end
    end

    -- sort by from level, row, col, then to level, row, col
    table.sort(edge_entries, function(a, b)
        if a.from_pos.level ~= b.from_pos.level then
            return a.from_pos.level < b.from_pos.level
        end
        if a.from_pos.row ~= b.from_pos.row then
            return a.from_pos.row < b.from_pos.row
        end
        if a.from_pos.col ~= b.from_pos.col then
            return a.from_pos.col < b.from_pos.col
        end
        if a.to_pos.level ~= b.to_pos.level then
            return a.to_pos.level < b.to_pos.level
        end
        if a.to_pos.row ~= b.to_pos.row then
            return a.to_pos.row < b.to_pos.row
        end
        return a.to_pos.col < b.to_pos.col
    end)

    -- helper function to check if edge is bidirectional
    local function is_bidirectional(graph, from_name, to_name, requirements)
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

    local printed_pairs = {}  -- track which bidirectional pairs we've already printed
    for _, edge_entry in ipairs(edge_entries) do
        local from = edge_entry.from
        local to = edge_entry.to
        local requirements = edge_entry.requirements or {}
    
        -- check if bidirectional
        local is_bidir = is_bidirectional(graph, from, to, requirements)
    
        -- for bidirectional edges, only print once (alphabetically first direction)
        if is_bidir then
            local pair_key = from < to and (from .. "-" .. to) or (to .. "-" .. from)
            if printed_pairs[pair_key] then
                goto continue
            end
            printed_pairs[pair_key] = true
        end
    
        local lock_status = edge_entry.locked_item and string.format("LOCKED (%s)", edge_entry.locked_item) or "open"
        local arrow = is_bidir and "<->" or "->"
    
        print(string.format("  L%d [%d,%d] %s %s L%d [%d,%d] %s: %s",
            edge_entry.from_pos.level, edge_entry.from_pos.row, edge_entry.from_pos.col, edge_entry.from,
            arrow,
            edge_entry.to_pos.level, edge_entry.to_pos.row, edge_entry.to_pos.col, edge_entry.to,
            lock_status))
    
        ::continue::
    end
end

function DebugHelper.print_item_placement(layout, graph, iteration, score)
    print(string.format("\nIteration %d (score: %.3f) - Layout with items:", iteration, score))
    
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

    -- print sorted entries
    for _, entry in ipairs(entries) do
        print(string.format("  L%d [%d,%d] %s: %s", entry.pos.level, entry.pos.row, entry.pos.col, entry.name, entry.item))
    end
end

function DebugHelper.print_skippable_warning(skippable)
    if #skippable > 0 then
        print("Warning: there are skippable items in this dungeon:")
        for _, item_info in ipairs(skippable) do
            print(string.format("  %s at %s", item_info.item, item_info.location))
        end
    end
end

function DebugHelper.print_final_report(best_score, best_report, Analyzer)
    print(string.format("\n=== Best Dungeon (score: %.3f) ===\n", best_score))
    print("")
    Analyzer.print_report(best_report)
end

function DebugHelper.print_builder_info(boss_room, excluded_rooms)
    local excluded_count = 0
    for _ in pairs(excluded_rooms) do
        excluded_count = excluded_count + 1
    end
    
    print(string.format("  BossRoom placed: %s", boss_room))
    print(string.format("  Excluded rooms: %d", excluded_count))
end

return DebugHelper