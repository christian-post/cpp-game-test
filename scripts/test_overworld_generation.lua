function execute()
    local WorldGraph             = require("dungeon.world_graph")
    local OverworldZoneGenerator = require("overworld.overworld_zone_generator")
    local OverworldGenerator     = require("overworld.overworld_generator")
    local OverworldBuilder       = require("overworld.overworld_builder")
    local TilemapModifier = require("overworld.tilemap_modifier")

    print("=== Overworld Generation Test ===\n")

    -- TODO random seeding for testing different outcomes
    set_dungeon_seed(os.time())

    -- some parameters for testing
    local width = 6
    local height = 8
    local item_pool = { "hookshot", "bombs", "bow", "boots", "boat" }

    -- steps 1-3: generate until reachable
    local layout -- contains: rows, cols, levels (only 1 for overworld), positions (mapping from node names to grid coordinates), and zones (the zone grid) 
    local edges -- { from, to, requirements }
    local zone_grid -- 2D grid (width x height), representing the "rooms" (=tilemaps) and their types (currently: mountain, field, lake, forest)
    local graph -- see WorldGraph for implementation details
    local excluded_rooms -- currently only the starting room

    local max_attempts = 20
    for attempt = 1, max_attempts do
        print(string.format("Attempt %d...", attempt))

        -- place the zones (terrain types have individual rules)
        zone_grid = OverworldZoneGenerator.generate(width, height)
        if not zone_grid then
            print("  Zone placement failed, retrying...")
            goto continue -- skip all further generation steps
        end

        -- construct the overall room layout (close some edges randomly based on zone transition rules)
        layout, edges = OverworldGenerator.generate(zone_grid, width, height)
        -- build the graph, which already includes item requirements for some of the edges
        graph, excluded_rooms = OverworldBuilder.build(WorldGraph, layout, edges, item_pool)

        -- make sure that none of the rooms are completely shut off
        if graph:test_reachability() then
            print(string.format("  Valid graph found on attempt %d\n", attempt))
            break
        end

        ::continue::

        if attempt == max_attempts then
            print("ERROR: Could not generate valid overworld after " .. max_attempts .. " attempts")
            return false
        end
    end

    -- step 4: forward fill rooms with items
    print("\nStep 4: Forward fill...")
    local success = graph:forward_fill(false)

    if not success then
        print("ERROR: Forward fill failed!")
        return false
    end

    --===========
    -- Debugging
    --===========

    -- print item placements for debugging
    print("\nItem placement:")
    local sorted_names = {}
    for name, _ in pairs(graph.nodes) do
        table.insert(sorted_names, name)
    end
    table.sort(sorted_names)

    for _, name in ipairs(sorted_names) do
        local node = graph.nodes[name]
        if node.value then
            print(string.format("  %s: %s", name, node.value))
        end
    end

    -- print zone grid
    print("Zone layout:")
    OverworldZoneGenerator.print_grid(zone_grid, width, height)

    -- print edge layout
    print("\nEdge layout:")
    OverworldGenerator.print_layout(zone_grid, width, height, edges)

    -- print graph info
    print("\nGraph info:")
    print(string.format("  Start: OW_%d_%d", layout.start_row, layout.start_col))

    local node_count = 0
    for _ in pairs(graph.nodes) do
        node_count = node_count + 1
    end
    print(string.format("  Nodes: %d", node_count))

    local excluded_count = 0
    for _ in pairs(excluded_rooms) do
        excluded_count = excluded_count + 1
    end
    print(string.format("  Excluded rooms: %d", excluded_count))

    print("\nReduced graph:")
    local reduced = graph:reduce()
    reduced:print_graph()

    -- =====================================
    -- Tilemap assignment
    -- =====================================

    TilemapModifier.process_overworld(zone_grid, edges, width, height)

    print("\n=== Overworld Generation Complete! ===")
end