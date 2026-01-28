function execute()
    local WorldGraph = require("dungeon.world_graph")
    local Analyzer = require("dungeon.dungeon_analyzer")
    local utils = require("lib.utils")

    local verbose = true
    
    local g = WorldGraph.new()
    -- level 0
    g:add_node("Room_0_0_0")
    g:add_node("Room_0_0_1")
    g:add_node("Room_0_0_2")
    g:add_node("Room_0_1_0")
    g:add_node("Room_0_1_1")
    g:add_node("Start")

    -- level 1
    g:add_node("Room_1_0_0")
    g:add_node("Room_1_0_1")
    g:add_node("BossRoom")
    g:add_node("Room_1_1_0")
    g:add_node("Room_1_1_1")
    g:add_node("Room_1_1_2")

    g:exclude_room("Start")
    g:exclude_room("BossRoom")
    -- exclude level connection rooms
    g:exclude_room("Room_0_1_0")
    g:exclude_room("Room_1_1_0")
    
    -- room connections
    g:add_edge("Room_0_0_0", "Room_0_1_0", {"key"})
    g:add_edge("Room_0_0_0", "Room_0_0_1")
    g:add_edge("Room_0_0_1", "Room_0_0_2")
    g:add_edge("Room_0_0_2", "Start")
    g:add_edge("Room_0_1_1", "Start")

    -- stairs
    g:add_edge("Room_0_1_0", "Room_1_1_0")

    g:add_edge("Room_1_0_0", "Room_1_0_1")
    g:add_edge("Room_1_0_1", "BossRoom", {"boss_key"})
    g:add_edge("Room_1_0_0", "Room_1_1_0")
    g:add_edge("Room_1_1_0", "Room_1_1_1", {"key"})
    g:add_edge("Room_1_1_1", "Room_1_1_2", {"weapon_sword"})
    
    g:set_start("Start")

    local itemPool = {"key", "key", "boss_key", "weapon_sword"}

    g:initialize_items(itemPool)

    -- Test if all nodes are reachable with all items
    if not g:test_reachability() then
        print("\nERROR: Cannot generate dungeon - fix graph structure first!")
        return nil
    end
    
    -- Quality-based generation
    local max_iterations = 20
    local target_score = 0.7
    local best_score = 0
    local best_placements = nil
    local best_report = nil
    local iteration = 0
    
    while iteration < max_iterations do
        iteration = iteration + 1
    
        -- Try to generate a valid dungeon (with retries for forward_fill)
        local max_attempts = 100
        local attempt = 0
        local success = false
    
        while not success and attempt < max_attempts do
            attempt = attempt + 1
        
            if attempt % 10 == 0 then
                print(string.format("  forward_fill attempt %d/%d...", attempt, max_attempts))
            end
        
            g:reset_items()
            g:initialize_items(itemPool)
            success = g:forward_fill(false)
        end
    
        if success then
            local score = Analyzer.challenge_score(g)
            
            if verbose then
                print(string.format("Iteration %d: score = %.3f", iteration, score))
            end
            
            -- Track best
            if score > best_score then
                best_score = score
                -- Save item placements (only nodes with items)
                best_placements = {}
                for name, node in pairs(g.nodes) do
                    if node.value then
                        best_placements[name] = node.value
                    end
                end
                -- Save the report
                best_report = Analyzer.generate_report(g)
            end
            
            -- Stop early if we hit target
            if score >= target_score then
                if verbose then
                    print(string.format("\nReached target score %.3f after %d iterations!", target_score, iteration))
                end
                break
            end
        else
            if verbose then
                print(string.format("Iteration %d: failed to place items", iteration))
            end
        end
    end
    
    -- Restore best placements to the graph
    if best_placements then
        -- clear all values first
        for name, node in pairs(g.nodes) do
            node.value = nil
        end
    
        for name, item in pairs(best_placements) do
            g.nodes[name].value = item
        end
        
        if verbose then
            print(string.format("\n=== Best Dungeon (score: %.3f) ===\n", best_score))
            
            print("Items placed:")
            for name, node in pairs(g.nodes) do
                if node.value then
                    print(string.format("  %s: %s", name, node.value))
                end
            end
            
            -- Use saved report
            Analyzer.print_report(best_report)
        end
        
        -- Return the graph with best placements
        return g, best_score, best_report
    else
        print("Failed to generate any valid dungeon!")
        return nil
    end
end