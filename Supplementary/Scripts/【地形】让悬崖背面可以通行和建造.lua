-- 【地形】让悬崖背面可以通行和建造
-- 找出所有悬崖背面的clear单元格
-- 将其替换为可以通行和建造的地形

-- ============ 预设：按地图类型区分 ============
-- 每项格式：{toTile, toSub}，即替换后使用的 tile/subtile。
-- 当前为占位数据 {0,0}，请按需自行修改。
local PRESETS = {
    ["TEMPERATE"] = {449, 0},
    ["SNOW"] = {735, 0},
    ["URBAN"] = {449, 0},
    ["NEWURBAN"] = {449, 0},
    ["LUNAR"] = {349, 0},
    ["DESERT"] = {417, 0},
}

-- ============ 执行替换 ============
local target = PRESETS[theater()]
if not target then
    print("警告：不支持的地图类型 " .. theater() .. "，未执行替换")
    return
end

save_undo() -- 记录撤销点，可Ctrl+Z回退

local changed = 0
local size = iso_size()
for x = 0, size - 1 do
    for y = 0, size - 1 do
        local cell = get_cell(x, y)
        if cell.tile >= 0 then
            -- 条件3：地表类型为clear
            local tile = get_tile_block(cell.tile, cell.subtile)
            if tile and tile.valid and tile.land_type == "clear" then
                -- 条件0：X,Y单元格的ramp类型必须为0
                if tile.ramp_type == 0 then
                    -- 条件1：(x+2,y+2)在地图内
                    if in_map(x + 2, y + 2) then
                        -- 条件2：后者的高度比前者高4格或以上
                        local cell2 = get_cell(x + 2, y + 2)
                        if cell2.height >= cell.height + 4 and cell2.tile >= 0 then
                            -- 条件4：(x+2,y+2)的地表类型必须为rock
                            local tile2 = get_tile_block(cell2.tile, cell2.subtile)
                            if tile2 and tile2.valid and tile2.land_type == "rock" then
                                cell.tile = target[1]
                                cell.subtile = target[2]
                                cell:apply()
                                changed = changed + 1
                            end
                        end
                    end
                end
            end
        end
    end
end

redraw_window()
update_minimap()
print("替换完成（" .. theater() .. "），共修改 " .. changed .. " 个格子")
