-- dungeon generation test script

local function generate_layout(config)
    local DungeonGenerator = require("dungeon.dungeon_generator")
    
    print("Step 1: Generating layout...")
    local layout, edges, stairway_rooms = DungeonGenerator.generate(config)
    
    return layout, edges, stairway_rooms, DungeonGenerator
end

local function build_graph(WorldGraph, layout, edges, stairway_rooms, item_pool)
    local DungeonBuilder = require("dungeon.dungeon_builder")
    local DebugHelper = require("dungeon.debug_helper")
    
    print("\nStep 2: Building graph...")
    update_progress("Building Dungeon Graph...")
    
    local graph, boss_room, excluded_rooms = DungeonBuilder.build(WorldGraph, layout, edges, stairway_rooms, item_pool)
    
    DebugHelper.print_builder_info(boss_room, excluded_rooms)
    
    return graph
end

local function test_reachability(graph, item_pool)
    print("\nStep 3: Testing reachability...")
    update_progress("Testing Reachability...")
    
    graph:initialize_items(item_pool)
    if not graph:test_reachability() then
        print("ERROR: Graph structure is invalid!")
        return false
    end
    
    return true
end

local function find_optimal_placement(graph, layout, item_pool, config)
    local Analyzer = require("dungeon.dungeon_analyzer")
    local DungeonUtils = require("dungeon.dungeon_utils")
    local DebugHelper = require("dungeon.debug_helper")
    
    print("\nStep 4: Finding optimal item placement...")
    update_progress("Placing Items...")
    
    local max_iterations = config.max_iterations or 10
    local target_score = config.target_score or 0.7
    local max_fill_attempts = config.max_fill_attempts or 25
    
    local best_score = 0
    local best_placements = nil
    local best_report = nil
    
    for iteration = 1, max_iterations do
        local success = false
        
        for attempt = 1, max_fill_attempts do
            -- tell the game that this is still running
            if not yield_to_engine() then
                print("INFO: Window closed during generation")
                return nil
            end
            
            graph:reset_items()
            graph:initialize_items(item_pool)
            success = graph:forward_fill(false)
            
            if success then
                local score = Analyzer.challenge_score(graph)
                
                -- check for skippable items
                local skippable = graph:find_skippable_items()
                local has_skippable_sword = DungeonUtils.has_skippable_item(skippable, "weapon_sword")
                
                if not has_skippable_sword then
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
                    print("  Iteration rejected: sword is skippable")
                end
            end
        end
        
        if success then
            local score = Analyzer.challenge_score(graph)
            print(string.format("  Iteration %d: score = %.3f", iteration, score))
            
            DebugHelper.print_item_placement(layout, graph, iteration, score)
            
            graph:reset_items()
            graph:initialize_items(item_pool)
            local skippable = graph:find_skippable_items()
            DebugHelper.print_skippable_warning(skippable)
            
            if score >= target_score and not DungeonUtils.has_skippable_item(skippable, "weapon_sword") then
                print(string.format("\nReached target score %.3f after %d iterations!", target_score, iteration))
                break
            end
        else
            print(string.format("  Iteration %d: failed to place items", iteration))
        end
    end
    
    return best_score, best_placements, best_report
end

local function restore_best_placement(graph, best_placements)
    graph:reset_items()
    
    for name, node in pairs(graph.nodes) do
        node.value = nil
    end
    
    for name, item in pairs(best_placements) do
        graph.nodes[name].value = item
    end
end

local function export_dungeon_data(layout, edges, graph, item_pool, stairway_rooms, dungeon_key, DungeonGenerator)
    local DungeonExporter = require("dungeon.dungeon_exporter")
    
    print("\nStep 6: Exporting dungeon data...")
    local dungeon_data = DungeonExporter.export(layout, edges, graph, item_pool, stairway_rooms, DungeonGenerator)
    DungeonExporter.append_to_dungeons(dungeon_data, "resources/dungeons.json", dungeon_key)
    return dungeon_data
end

local function generate_base_rooms_if_needed()
    local GenerateBaseRooms = require("dungeon.generate_base_rooms")
    
    print("\nStep 7: Checking base room tilemaps...")
    update_progress("Generating Base Rooms...")
    
    -- TODO comment back in when finished debugging

    --[[
        local base_check = io.open("resources/tilemaps/base/room_0001.json", "r")
    if not base_check then
        print("  Base rooms not found, generating...")
        GenerateBaseRooms.generate("resources/tilemaps/empty_floor_new.json", "resources/tilemaps/base")
    else
        base_check:close()
        print("  Base rooms already exist, skipping generation")
    end

    ]]
    GenerateBaseRooms.generate("resources/tilemaps/empty_floor_new.json", "resources/tilemaps/base")
end

local function process_tilemaps(dungeon_key, output_dir)
    local TilemapModifier = require("dungeon.tilemap_modifier")
    local ObjectTemplates = require("dungeon.object_templates")
    
    print("\nStep 9: Processing tilemaps...")
    update_progress("Processing Tilemaps...")
    
    TilemapModifier.process_dungeon("resources/dungeons.json", dungeon_key, "resources/tilemaps/base", output_dir, ObjectTemplates)
end

function execute()
    local WorldGraph = require("dungeon.world_graph")
    local Analyzer = require("dungeon.dungeon_analyzer")
    local DebugHelper = require("dungeon.debug_helper")
    local HintGenerator = require("dungeon.hint_generator")
    
    print("=== Modular Dungeon Generation Test ===\n")
    dungeon_generation_start()
    
    local dungeon_key = "lua_dungeon"
    local output_dir = "resources/tilemaps/generated/" .. dungeon_key
    
    -- setup output directory
    filesystem.create_directory(output_dir)
    filesystem.clear_directory(output_dir)
    
    -- step 1: generate layout
    local layout_config = {
        rows = 2,
        cols = 3,
        levels = 2,
        start_row = 1,
        start_col = 2,
        dead_end_chance = 0.5,
        growth_rate = 4.0
    }
    local layout, edges, stairway_rooms, DungeonGenerator = generate_layout(layout_config)
    DebugHelper.print_generation_summary(layout, edges)
    
    -- step 2: build graph
    local item_pool = {"key", "key", "key", "boss_key", "weapon_sword"}
    local graph = build_graph(WorldGraph, layout, edges, stairway_rooms, item_pool)
    
    -- print debug information
    DebugHelper.print_layout(layout, graph)
    DebugHelper.print_door_patterns(layout, edges, DungeonGenerator)
    DebugHelper.print_edges(layout, edges, graph)
    
    -- step 3: test reachability
    if not test_reachability(graph, item_pool) then
        dungeon_generation_complete(false)
        return false
    end
    
    -- step 4: find optimal item placement
    local placement_config = {
        max_iterations = 10, -- how often the score gets evaluated
        target_score = 0.7, -- dungeon score (0 to 1)
        max_fill_attempts = 25 -- max tries within one evaluation cycle
    }
    local best_score, best_placements, best_report = find_optimal_placement(graph, layout, item_pool, placement_config)
    
    -- step 5: finalize and export
    if best_placements then
        restore_best_placement(graph, best_placements)
        DebugHelper.print_final_report(best_score, best_report, Analyzer)
        
        local dungeon_data = export_dungeon_data(layout, edges, graph, item_pool, stairway_rooms, dungeon_key, DungeonGenerator)
        generate_base_rooms_if_needed()
        print("display name" .. dungeon_data.display_name)
        HintGenerator.generate(graph, layout, dungeon_key, dungeon_data.display_name)
        process_tilemaps(dungeon_key, output_dir)
        
        print("\n=== Dungeon Generation Complete! ===")
        dungeon_generation_complete(true)
        
        return graph, layout, best_score, best_report
    else
        print("\nFailed to generate any valid dungeon!")
        dungeon_generation_complete(false)

        return false
    end
end