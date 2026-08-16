-- 正确使用水上隧道
-- 需要在已经摆放了隧道后使用。
-- 若摆放了新的，则需要重新执行替换地形
-- 1) 替换地形：把隧道口的Road替换为Water
-- 2) 修改INI：写入 [Tunnel] 节；
--    修改所有船只的移动区域

-- ============ 预设：按地图类型区分的替换表 ============
-- 每项格式：{fromTile, fromSub, startTile, endTile}
-- 含义：把所有 tile==fromTile 且 subtile==fromSub 的格子替换为
--       startTile~endTile 区间内随机一个tile，subtile固定为0。
local PRESETS = {
    ["TEMPERATE"] = {
        {936, 4, 322, 326},
        {936, 7, 322, 326},
        {936, 10, 322, 326},
        {936, 5, 322, 326},
        {936, 8, 322, 326},
        {936, 11, 322, 326},
        {937, 5, 322, 326},
        {937, 6, 322, 326},
        {937, 7, 322, 326},
        {937, 9, 322, 326},
        {937, 10, 322, 326},
        {937, 11, 322, 326},
    },
    ["SNOW"] = {
        {1598, 4, 322, 326},
        {1598, 7, 322, 326},
        {1598, 10, 322, 326},
        {1598, 5, 322, 326},
        {1598, 8, 322, 326},
        {1598, 11, 322, 326},
        {1599, 5, 322, 326},
        {1599, 6, 322, 326},
        {1599, 7, 322, 326},
        {1599, 9, 322, 326},
        {1599, 10, 322, 326},
        {1599, 11, 322, 326},
    },
    ["URBAN"] = {
        {1256, 4, 322, 326},
        {1256, 7, 322, 326},
        {1256, 10, 322, 326},
        {1256, 5, 322, 326},
        {1256, 8, 322, 326},
        {1256, 11, 322, 326},
        {1257, 5, 322, 326},
        {1257, 6, 322, 326},
        {1257, 7, 322, 326},
        {1257, 9, 322, 326},
        {1257, 10, 322, 326},
        {1257, 11, 322, 326},
    },
    ["NEWURBAN"] = {
        {1672, 4, 322, 326},
        {1672, 7, 322, 326},
        {1672, 10, 322, 326},
        {1672, 5, 322, 326},
        {1672, 8, 322, 326},
        {1672, 11, 322, 326},
        {1673, 5, 322, 326},
        {1673, 6, 322, 326},
        {1673, 7, 322, 326},
        {1673, 9, 322, 326},
        {1673, 10, 322, 326},
        {1673, 11, 322, 326},
        {1676, 4, 322, 326},
        {1676, 7, 322, 326},
        {1676, 10, 322, 326},
        {1676, 5, 322, 326},
        {1676, 8, 322, 326},
        {1676, 11, 322, 326},
        {1677, 5, 322, 326},
        {1677, 6, 322, 326},
        {1677, 7, 322, 326},
        {1677, 9, 322, 326},
        {1677, 10, 322, 326},
        {1677, 11, 322, 326},
    },
    ["DESERT"] = {
        {983, 4, 294, 298},
        {983, 7, 294, 298},
        {983, 10, 294, 298},
        {983, 5, 294, 298},
        {983, 8, 294, 298},
        {983, 11, 294, 298},
        {984, 5, 294, 298},
        {984, 6, 294, 298},
        {984, 7, 294, 298},
        {984, 9, 294, 298},
        {984, 10, 294, 298},
        {984, 11, 294, 298},
    },
}

-- ============ 弹出对话框 ============
local dlg = LuaDialog:new("批量替换地形 / 修改INI", true)

dlg:add_checkbox("replace_terrain", "替换地形", true)
dlg:add_checkbox("modify_ini", "修改INI", true)

local result = dlg:do_modal()
if not result then
    print("已取消操作")
    return
end

local do_terrain = result.replace_terrain
local do_ini = result.modify_ini

if not do_terrain and not do_ini then
    print("未选择任何操作")
    return
end

-- ============ 1. 替换地形 ============
if do_terrain then
    local preset = PRESETS[theater()]
    if not preset then
        print("警告：不支持的地图类型 " .. theater() .. "，跳过地形替换")
    else
        -- 构建查找表：key = fromTile*100000 + fromSub
        local lookup = {}
        for i, e in ipairs(preset) do
            lookup[e[1] * 100000 + e[2]] = {e[3], e[4]}
        end
        if next(lookup) then
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
                                cell.tile = math.random(target[1], target[2])
                                cell.subtile = 0
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
        else
            print("提示：" .. theater() .. " 的替换表为空，未执行替换")
        end
    end
end

-- ============ 2. 修改INI ============
if do_ini then
    create_snapshot() -- 保存快照以便恢复

    -- 2.1 写入 [Tunnel] 节
    write_string("Tunnel", "Foot", "1")
    write_string("Tunnel", "Track", "1")
    write_string("Tunnel", "Wheel", "1")
    write_string("Tunnel", "Float", "1")
    write_string("Tunnel", "FloatBeach", "1")
    write_string("Tunnel", "Hover", "1")
    write_string("Tunnel", "Amphibious", "1")
    write_string("Tunnel", "Buildable", "0")
    print("[Tunnel] 节已写入")

    -- 2.2 遍历 VehicleTypes，处理水栖单位
    local handled = {}
    local count = 0
    local vehicles = get_values("VehicleTypes", "rules+map")
    for i, id in ipairs(vehicles) do
        if id ~= "" and not handled[id] then
            handled[id] = true
            if get_bool(id, "Naval", false, "rules+map") then
                if get_string(id, "MovementZone", "", "rules+map") == "Water" then
                    write_string(id, "MovementZone", "AmphibiousDestroyer")
                    write_string(id, "MovementRestrictedTo", "0")
                    count = count + 1
                    print("已修改: " .. id)
                end
            end
        end
    end
    print("INI修改完成，共修改 " .. count .. " 个单位")
end

print("全部操作执行完毕")
