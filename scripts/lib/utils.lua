local M = {}

function M.shallowCopy(t)
    local copy = {}
    for i, v in ipairs(t) do
        copy[i] = v
    end
    return copy
end

function M.deep_copy(obj)
    if type(obj) ~= "table" then
        return obj
    end
    
    local copy = {}
    for k, v in pairs(obj) do
        copy[k] = M.deep_copy(v)
    end
    return copy
end

function M.has_value (table, val)
    for index, value in ipairs(table) do
        if value == val then
            return true
        end
    end

    return false
end

function M.shuffle(list)
    -- fisher-yates shuffle
    for i = #list, 2, -1 do
        local j =dungeon_random(1, i)
        list[i], list[j] = list[j], list[i]
    end
end

function M.loadJSON(path)
    local file = io.open(path, "r")
    if not file then
        error("Could not open file: " .. path)
    end
    local content = file:read("*all")
    file:close()
    return json.decode(content)
end

function M.saveJSON(path, data)
    local out_file = io.open(path, "w")
    if not out_file then
        error("Could not write file: " .. path)
    end
    out_file:write(json.encode(data, 2))
    out_file:close()
end

function M.manhattan_dist(a, b)
    return math.abs(a.row - b.row) + math.abs(a.col - b.col)
end


return M