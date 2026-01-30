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
    
    print(string.format("  BossRoom placed: %s", boss_room))
    print(string.format("  Excluded rooms: %d", 
        (function()
            local count = 0
            for _ in pairs(excluded_rooms) do
                count = count + 1
            end
            return count
        end)()))
    
    -- step 3: test reachability
    print("\nStep 3: Testing reachability...")
    graph:initialize_items(item_pool)
    if not graph:test_reachability() then
        print("ERROR: Graph structure is invalid!")
        return nil
    end
    
    -- step 4: quality-based item placement
    print("\nStep 4: Finding optimal item placement...")
    local max_iterations = 20
    local target_score = 0.7
    local best_score = 0
    local best_placements = nil
    local best_report = nil
    
    for iteration = 1, max_iterations do
        -- try forward_fill with retries
        local max_attempts = 100
        local success = false
        
        for attempt = 1, max_attempts do
            graph:reset_items()
            graph:initialize_items(item_pool)
            success = graph:forward_fill(false)
            
            if success then
                break
            end
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
        for name, node in pairs(graph.nodes) do
            node.value = nil
        end
        
        for name, item in pairs(best_placements) do
            graph.nodes[name].value = item
        end
        
        print(string.format("\n=== Best Dungeon (score: %.3f) ===\n", best_score))
        
        print("Layout:")
        for name, pos in pairs(layout.positions) do
            local item = graph.nodes[name].value or "empty"
            print(string.format("  L%d [%d,%d] %s: %s", pos.level, pos.row, pos.col, name, item))
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
        
        print("")
        Analyzer.print_report(best_report)
        
        -- step 6: export dungeon data
        print("\nStep 6: Exporting dungeon data...")
        local dungeon_data = DungeonExporter.export(layout, edges, graph, item_pool, stairway_rooms, DungeonGenerator)
        DungeonExporter.append_to_dungeons(dungeon_data, "resources/dungeons.json", "lua_dungeon")
        DungeonExporter.save_to_file(dungeon_data, "resources/test_lua_dungeon.json")
        
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
        return nil
    end
end