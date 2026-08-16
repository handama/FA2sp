local PRESETS = {
    ["TEMPERATE"] = {
        {389, 0, 370, 1},
        {392, 0, 368, 1},
    },
    ["SNOW"] = {
        {389, 0, 370, 1},
        {392, 0, 368, 1},
        {883, 0, 370, 1},
        {886, 0, 368, 1},
        {1461, 0, 370, 1},
        {1464, 0, 368, 1},
        {1764, 0, 370, 1},
        {1767, 0, 368, 1},
    },
    ["URBAN"] = {
        {389, 0, 370, 1},
        {392, 0, 368, 1},
    },
    ["NEWURBAN"] = {
        {389, 0, 370, 1},
        {392, 0, 368, 1},
        {1111, 0, 370, 1},
        {1114, 0, 368, 1},
    },
    ["LUNAR"] = {
        {520, 0, 341, 1},
        {523, 0, 339, 1},
    },
    ["DESERT"] = {
        {361, 0, 342, 1},
        {364, 0, 341, 1},
    },
}

-- ============ 按地图类型执行替换 ============
local preset = PRESETS[theater()]
if not preset then
    print("警告：不支持的地图类型 " .. theater() .. "，未执行替换")
    return
end

-- 构建查找表：key = fromTile*100000 + fromSub
local lookup = {}
for i, e in ipairs(preset) do
    lookup[e[1] * 100000 + e[2]] = {e[3], e[4]}
end

if not next(lookup) then
    print("提示：" .. theater() .. " 的替换表为空，未执行替换")
    return
end

save_undo() -- 记录撤销点，可Ctrl+Z回退

local changed = 0
local size = iso_size()
for x = 0, size - 1 do
    for y = 0, size - 1 do
        if in_map(x, y) then
            local cell = get_cell(x, y)
            if cell.tile >= 0 then
                local target = lookup[cell.tile * 100000 + cell.subtile]
                if target then
                    cell.tile = target[1]
                    cell.subtile = target[2]
                    cell:apply()
                    changed = changed + 1
                end
            end
        end
    end
end

redraw_window()
update_minimap()
print("地形替换完成（" .. theater() .. "），共修改 " .. changed .. " 个格子")
