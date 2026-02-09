local M = {}

function M.shallowCopy(t)
    local copy = {}
    for i, v in ipairs(t) do
        copy[i] = v
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

return M