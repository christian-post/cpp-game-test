-- builds the first, simple WorldGraph from an overworld layout and edges
local WorldUtils = require("lib.world_utils")
local OverworldBuilder = {}

local function build_graph_from_layout(WorldGraph, layout, edges)
    local graph = WorldGraph.new()

    for name, _ in pairs(layout.positions) do
        graph:add_node(name)
    end

    for _, edge in ipairs(edges) do
        if #edge.requirements == 0 then
            graph:add_one_way_edge(edge.from, edge.to, {})
        else
            graph:add_one_way_edge(edge.from, edge.to, edge.requirements)
        end
    end

    return graph
end

function OverworldBuilder.build(WorldGraph, layout, edges, item_pool)
    local graph = build_graph_from_layout(WorldGraph, layout, edges)

    local start_name = WorldUtils.node_name(layout.start_row, layout.start_col)
    graph:set_start(start_name)
    graph:exclude_room(start_name)

    graph:initialize_items(item_pool)

    local excluded_rooms = { [start_name] = true }

    return graph, excluded_rooms
end

return OverworldBuilder