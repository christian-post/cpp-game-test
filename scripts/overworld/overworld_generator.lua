-- generates overworld layout from a zone grid, building edges
-- constrained by zone transition rules
local utils = require("lib.utils")
local WorldUtils = require("lib.world_utils")
local OverworldGenerator = {}

-- general zone transition rules
-- if "max" not nil, it limits the number of transitions that can occur. max = 0 means the edge is always shut off
-- when multiple rules are given, the algorithm always exhausts the first one.
-- For example:
--[[
    mountain_to_mountain = {{ max = 2, requirement = "hookshot" },
                            { max = 3, requirement = nil        }}
    This ruleset means there will be two edges that require the hookshot for traversal.
    Then, three other edges will be completely open.
    All other remaining edges between mountain zones will be closed.
]]
-- IMPORTANT: when there are multiple rules, the first rule can't have max=0 or it will break the connectivity check
-- max = nil means that the edge is always open
local default_transition_rules = {
    field_to_field       = { { max = nil, requirement = nil } },
    field_to_forest      = { { max = 2,   requirement = nil } },
    forest_to_forest     = { { max = nil, requirement = nil } },
    field_to_mountain    = { { max = 2,   requirement = "bombs" } },
    mountain_to_mountain = { { max = 3,   requirement = "hookshot" },
                             { max = nil, requirement = nil } },
    field_to_lake        = { { max = 2,   requirement = "boat" } },
    lake_to_lake         = { { max = nil, requirement = "boat" } },
    forest_to_mountain   = { { max = 1,   requirement = nil } },
    forest_to_lake       = { { max = 0,   requirement = nil } },
    mountain_to_lake     = { { max = 0,   requirement = nil } },
    town_to_town         = { { max = nil, requirement = nil } },
    town_to_field        = { { max = nil, requirement = nil } },
    town_to_forest       = { { max = 0,   requirement = nil } },
    town_to_lake         = { { max = 0,   requirement = nil } },
    town_to_mountain     = { { max = 0,   requirement = nil } },
}

local function get_transition_rules(rules, zone_a, zone_b)
    local key = zone_a .. "_to_" .. zone_b
    local reverse_key = zone_b .. "_to_" .. zone_a
    -- try the key, e.g. "field_to_mountain" or, if that fails try the reverse key
    -- (so that I don't have to include duplicates for every reverse edge)
    -- return the rules and the actual key used in the rules table
    return rules[key] or rules[reverse_key], rules[key] and key or reverse_key
end

local function build_candidate_edges(zone_grid, width, height, rules)
    -- collect all valid adjacent cell pairs grouped by transition type
    local candidates_by_type = {}

    local directions = { {0, 1}, {1, 0} }  -- right and down only to avoid duplicates

    for row = 0, height - 1 do
        for col = 0, width - 1 do
            for _, dir in ipairs(directions) do
                -- get neighboring row and column
                local nr = row + dir[1]
                local nc = col + dir[2]

                if nr < height and nc < width then
                    local zone_a = zone_grid[row][col]
                    local zone_b = zone_grid[nr][nc]
                    -- retrieves the zone edge rules from the transition_rules table
                    -- the returned key can either be "[zone_a]_to_[zone_b]" or the reverse case, 
                    -- depending on what's used in the table
                    local zone_rules, transition_key = get_transition_rules(rules, zone_a, zone_b)

                    if zone_rules and (zone_rules[1].max == nil or zone_rules[1].max > 0) then
                        if not candidates_by_type[transition_key] then
                            candidates_by_type[transition_key] = {}
                        end

                        table.insert(candidates_by_type[transition_key], {
                            from = WorldUtils.node_name(row, col),
                            to = WorldUtils.node_name(nr, nc),
                            requirement = zone_rules[1].requirement
                        })
                    end
                end
            end
        end
    end

    return candidates_by_type
end

local function select_edges(candidates_by_type, rules)
    local selected = {}

    -- sort keys so that the order is deterministic
    local keys = {}
    for key, _ in pairs(candidates_by_type) do
        table.insert(keys, key)
    end
    table.sort(keys)

    for _, transition_key in ipairs(keys) do
        local candidates = candidates_by_type[transition_key]
        utils.shuffle(candidates)

        local tiers = rules[transition_key] -- { max usage, requirements }
        local remaining = candidates

        for _, tier in ipairs(tiers) do
            if #remaining == 0 then
                break
            end

            local limit = tier.max ~= nil and tier.max or #remaining
            local count = math.min(limit, #remaining)

            for i = 1, count do
                table.insert(selected, {
                    from = remaining[i].from,
                    to = remaining[i].to,
                    requirement = tier.requirement
                })
            end

            -- pass leftover candidates to next tier
            local next_remaining = {}
            for i = count + 1, #remaining do
                table.insert(next_remaining, remaining[i])
            end
            remaining = next_remaining
        end
    end

    return selected
end

local function ensure_connectivity(all_node_names, selected_edges, zone_grid, width, height, rules)
    -- build adjacency from selected edges
    local adjacency = {}
    for _, name in ipairs(all_node_names) do
        adjacency[name] = {}
    end
    for _, edge in ipairs(selected_edges) do
        table.insert(adjacency[edge.from], edge.to)
        table.insert(adjacency[edge.to], edge.from)
    end

    -- BFS from first node
    local visited = {}
    local queue = { all_node_names[1] }
    visited[all_node_names[1]] = true
    local qi = 1
    while qi <= #queue do
        local current = queue[qi]
        qi = qi + 1
        for _, neighbor in ipairs(adjacency[current]) do
            if not visited[neighbor] then
                visited[neighbor] = true
                table.insert(queue, neighbor)
            end
        end
    end

    -- find isolated nodes and connect to nearest visited neighbor
    for _, name in ipairs(all_node_names) do
        if not visited[name] then
            local row, col = name:match("OW_(%d+)_(%d+)")
            row, col = tonumber(row), tonumber(col)

            local neighbors = { {row - 1, col}, {row + 1, col}, {row, col - 1}, {row, col + 1} }
            for _, n in ipairs(neighbors) do
                local nr, nc = n[1], n[2]
                if nr >= 0 and nr < height and nc >= 0 and nc < width then
                    local neighbor_name = WorldUtils.node_name(nr, nc)
                    if visited[neighbor_name] then
                        local zone_a = zone_grid[row][col]
                        local zone_b = zone_grid[nr][nc]
                        local zone_rules, _ = get_transition_rules(rules, zone_a, zone_b)
                        local req = zone_rules and zone_rules[1].requirement or nil

                        local edge = { from = name, to = neighbor_name, requirement = req }
                        table.insert(selected_edges, edge)

                        visited[name] = true
                        table.insert(adjacency[name], neighbor_name)
                        table.insert(adjacency[neighbor_name], name)

                        print(string.format("  connectivity fallback: added edge %s <-> %s", name, neighbor_name))
                        break
                    end
                end
            end
        end
    end

    return selected_edges
end

local function find_start(zone_grid, width, height)
    local field_cells = {}
    for row = 0, height - 1 do
        for col = 0, width - 1 do
            if zone_grid[row][col] == "field" then
                table.insert(field_cells, {row = row, col = col})
            end
        end
    end

    if #field_cells == 0 then
        error("No field cells available for start position")
    end

    return field_cells[dungeon_random(1, #field_cells)]
end

function OverworldGenerator.generate(zone_grid, width, height, config)
    config = config or {}
    -- see default_transition_rules at the top of this script
    local rules = config.transition_rules or default_transition_rules

    -- build node name list ("OW_[row]_[col]") in deterministic order
    -- for now, each zone grid cell gets turned into one node
    local all_node_names = {}
    for row = 0, height - 1 do
        for col = 0, width - 1 do
            table.insert(all_node_names, WorldUtils.node_name(row, col))
        end
    end

    -- collect candidate edges grouped by transition type
    local candidates_by_type = build_candidate_edges(zone_grid, width, height, rules)

    -- apply max restrictions (how often an edge can be placed overall) and select edges
    local selected_edges = select_edges(candidates_by_type, rules)

    -- ensure full connectivity with fallback edges
    selected_edges = ensure_connectivity(all_node_names, selected_edges, zone_grid, width, height, rules)

    -- build layout compatible with DungeonBuilder/WorldGraph
    local layout = {
        rows = height,
        cols = width,
        levels = 1,
        positions = {},
        zones = zone_grid
    }
    for row = 0, height - 1 do
        for col = 0, width - 1 do
            layout.positions[WorldUtils.node_name(row, col)] = { row = row, col = col, level = 0 }
        end
    end

    -- pick random field cell as start
    local start_pos  = find_start(zone_grid, width, height)
    layout.start_row = start_pos.row
    layout.start_col = start_pos.col

    -- build edges table in dungeon format (bidirectional pairs)
    local edges = {}
    for _, edge in ipairs(selected_edges) do
        local req = edge.requirement and { edge.requirement } or {}
        table.insert(edges, { from = edge.from, to = edge.to,   requirements = req })
        table.insert(edges, { from = edge.to,   to = edge.from, requirements = req })
    end

    -- overworld has no stairway rooms
    local stairway_rooms = {}

    return layout, edges, stairway_rooms
end

function OverworldGenerator.print_layout(zone_grid, width, height, edges)
    -- just for debugging
    local zone_chars = { mountain = "M", lake = "L", forest = "F", town = "T", field = "." }
    -- build edge lookup and collect abbreviated requirements
    local edge_set = {}
    local req_seen = {}
    local req_list = {}
    for _, edge in ipairs(edges) do
        local req = edge.requirements
        edge_set[edge.from .. "->" .. edge.to] = req
        if req and #req > 0 then
            local name = req[1]
            if not req_seen[name] then
                req_seen[name] = true
                req_list[#req_list + 1] = name
            end
        end
    end
    table.sort(req_list)
    -- group requirements by first letter
    local by_letter = {}
    for _, name in ipairs(req_list) do
        local letter = string.sub(name, 1, 1):upper()
        by_letter[letter] = by_letter[letter] or {}
        table.insert(by_letter[letter], name)
    end
    -- assign codes: bare letter when unique, letter+index on collision
    local req_code = {}
    for letter, names in pairs(by_letter) do
        table.sort(names)
        if #names == 1 then
            req_code[names[1]] = letter
        else
            for i, name in ipairs(names) do
                req_code[name] = letter .. i
            end
        end
    end
    -- print legend
    utils.print_file("legend:")
    utils.print_file("  zones: M=mountain L=lake F=forest T=town .=field")
    utils.print_file("  edges: +=no requirement  'x'=no edge")
    if #req_list > 0 then
        utils.print_file("  requirements:")
        for _, name in ipairs(req_list) do
            utils.print_file("    " .. req_code[name] .. "=" .. name)
        end
    end
    local function edge_char(a, b)
        local reqs = edge_set[a .. "->" .. b]
        if not reqs then
            return "x"
        end
        if #reqs == 0 then
            return "+"
        end
        return req_code[reqs[1]] or string.sub(reqs[1], 1, 1):upper()
    end
    -- pad a cell to a fixed width of two so 1- and 2-char codes stay aligned
    local function pad_cell(s)
        if #s >= 2 then
            return s
        end
        return s .. " "
    end
    for row = 0, height - 1 do
        local line = "  "
        for col = 0, width - 1 do
            line = line .. pad_cell(zone_chars[zone_grid[row][col]] or "?")
            if col < width - 1 then
                line = line .. pad_cell(edge_char(WorldUtils.node_name(row, col), WorldUtils.node_name(row, col + 1)))
            end
        end
        utils.print_file(line)
        if row < height - 1 then
            local edge_line = "  "
            for col = 0, width - 1 do
                edge_line = edge_line .. pad_cell(edge_char(WorldUtils.node_name(row, col), WorldUtils.node_name(row + 1, col)))
                if col < width - 1 then
                    edge_line = edge_line .. "  "
                end
            end
            utils.print_file(edge_line)
        end
    end
end

return OverworldGenerator