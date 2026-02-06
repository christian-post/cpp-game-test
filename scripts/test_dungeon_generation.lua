function execute()
    local DungeonGenerator = require("dungeon.dungeon_generator")
    local DungeonBuilder = require("dungeon.dungeon_builder")
    local DungeonExporter = require("dungeon.dungeon_exporter")
    local WorldGraph = require("dungeon.world_graph")
    local Analyzer = require("dungeon.dungeon_analyzer")
    local GenerateBaseRooms = require("dungeon.generate_base_rooms")
    local TilemapModifier = require("dungeon.tilemap_modifier")
    local ObjectTemplates = require("dungeon.object_templates")

    print("=== RNG Test ===")
    print("Random 1: " .. math.random())
    print("Random 2: " .. math.random())
    print("Random 3: " .. math.random() .. "\n")
    
    print("=== Modular Dungeon Generation Test ===\n")
    
    -- step 1: generate layout
    print("Step 1: Generating layout...")
    local layout, edges, stairway_rooms = DungeonGenerator.generate({
        rows = 2,
        cols = 3,
        levels = 2,
        start_row = 1,
        start_col = 2,
        dead_end_chance = 0.5,
        growth_rate = 4.0
    })
    
    print(string.format("  Generated %d rooms across %d levels", 
        (function()
            local count = 0
            for _ in pairs(layout.positions) do
                count = count + 1
            end
            return count
        end)(), layout.levels))
    print(string.format("  Created %d edges", #edges))
    
    -- step 2: build graph
    print("\nStep 2: Building graph...")
    local item_pool = {"key", "key", "key", "boss_key", "weapon_sword"}
    local graph, boss_room, excluded_rooms = DungeonBuilder.build(
        WorldGraph,
        layout,
        edges,
        stairway_rooms,
        item_pool
    )

    -- print everything for debugging
    print(string.format("  BossRoom placed: %s", boss_room))
    print(string.format("  Excluded rooms: %d", 
        (function()
            local count = 0
            for _ in pairs(excluded_rooms) do
                count = count + 1
            end
            return count
        end)()))

    print("\nLayout:")
    -- collect entries into a table
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

    print("\nEdges:")
    -- collect edges into a table for sorting
    local edge_entries = {}
    for _, edge in ipairs(edges) do
        local from_pos = layout.positions[edge[1]]
        local to_pos = layout.positions[edge[2]]
    
        -- skip edges where positions aren't found
        if from_pos and to_pos then
            local locked_item = nil
            if edge[3] and #edge[3] > 0 then
                locked_item = edge[3][1]
            end
            table.insert(edge_entries, {
                from = edge[1],
                to = edge[2],
                from_pos = from_pos,
                to_pos = to_pos,
                locked_item = locked_item
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

    -- print sorted edges
    for _, edge_entry in ipairs(edge_entries) do
        local lock_status = edge_entry.locked_item and string.format("LOCKED (%s)", edge_entry.locked_item) or "open"
        print(string.format("  L%d [%d,%d] %s <-> L%d [%d,%d] %s: %s",
            edge_entry.from_pos.level, edge_entry.from_pos.row, edge_entry.from_pos.col, edge_entry.from,
            edge_entry.to_pos.level, edge_entry.to_pos.row, edge_entry.to_pos.col, edge_entry.to,
            lock_status))
    end
    
    -- step 3: test reachability
    print("\nStep 3: Testing reachability...")
    graph:initialize_items(item_pool)
    if not graph:test_reachability() then
        print("ERROR: Graph structure is invalid!")
        return nil
    end
    
    -- step 4: quality-based item placement
    print("\nStep 4: Finding optimal item placement...")
    local max_iterations = 1 -- how often it evaluates the dungeon score
    local target_score = 0.7
    local best_score = 0
    local best_placements = nil
    local best_report = nil
    
    for iteration = 1, max_iterations do
        -- try forward_fill with retries
        local max_fill_attempts = 20
        local success = false
    
        for attempt = 1, max_fill_attempts do
            -- tell the Game that this is still running
            if not yield_to_engine() then
                print("INFO: Window closed during generation")
                return false
            end

            graph:reset_items()
            graph:initialize_items(item_pool)
            success = graph:forward_fill(false)
    
            if success then
                -- check if the completed dungeon is solvable
                local is_solvable, trap_state = graph:is_solvable(false)
                if is_solvable then
                    break  -- found a good dungeon
                else
                    if trap_state then
                        print(string.format("  TRAP STATE DETAILS:"))
                        print(string.format("    Location: %s", trap_state.node.name))
                        print(string.format("    Inventory:"))
                        for item, count in pairs(trap_state.inventory) do
                            if type(count) == "number" then
                                print(string.format("      %s x%d", item, count))
                            else
                                print(string.format("      %s", item))
                            end
                        end
                        print(string.format("    Collected:"))
                        for loc, _ in pairs(trap_state.collected) do
                            print(string.format("      %s", loc))
                        end
                    end
                    print(string.format("  Attempt %d: placement complete but has trap, retrying...", attempt))
                    success = false
                end
            end
        end

        -- print final layout with placed items
        print("Layout with items:")

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
    
        if success then
            local score = Analyzer.challenge_score(graph)
            print(string.format("  Iteration %d: score = %.3f", iteration, score))
        
            if score > best_score then
                best_score = score
                best_placements = {}
                for name, node in pairs(graph.nodes) do
                    if node.value then
                        best_placements[name] = node.value
                    end
                end
                best_report = Analyzer.generate_report(graph)
            end
        
            if score >= target_score then
                print(string.format("\nReached target score %.3f after %d iterations!", target_score, iteration))
                break
            end
        else
            print(string.format("  Iteration %d: failed to place items", iteration))
        end
    end
    
    -- step 5: restore best placement
    if best_placements then
        graph:reset_items()

        for name, node in pairs(graph.nodes) do
            node.value = nil
        end
        
        for name, item in pairs(best_placements) do
            graph.nodes[name].value = item
        end
        
        print(string.format("\n=== Best Dungeon (score: %.3f) ===\n", best_score))
        
        print("")
        Analyzer.print_report(best_report)
        
        -- step 6: export dungeon data
        print("\nStep 6: Exporting dungeon data...")
        local dungeon_data = DungeonExporter.export(layout, edges, graph, item_pool, stairway_rooms, DungeonGenerator)
        DungeonExporter.append_to_dungeons(dungeon_data, "resources/dungeons.json", "lua_dungeon")
        
        -- step 7: generate base room tilemaps (only if needed)
        print("\nStep 7: Checking base room tilemaps...")
        -- check if base rooms exist, if not generate them
        local base_check = io.open("resources/tilemaps/base/room_0001.json", "r")
        if not base_check then
            print("  Base rooms not found, generating...")
            GenerateBaseRooms.generate(
                "resources/tilemaps/empty_floor.json",
                "resources/tilemaps/base"
            )
        else
            base_check:close()
            print("  Base rooms already exist, skipping generation")
        end
        
        -- step 8: process tilemaps (add items, stairs, etc.)
        print("\nStep 8: Processing tilemaps...")
        TilemapModifier.process_dungeon(
            "resources/dungeons.json",
            "lua_dungeon",
            "resources/tilemaps/base",
            "resources/tilemaps/generated",
            ObjectTemplates
        )
        
        print("\n=== Dungeon Generation Complete! ===")
        
        return graph, layout, best_score, best_report
    else
        print("\nFailed to generate any valid dungeon!")
        return false
    end
end