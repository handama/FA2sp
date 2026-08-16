--【地形】批量替换地形块
-- 仅作用于多选选中的单元格
-- 按预设将一段地形索引整体替换为另一段，例如 100~105 → 200~205（逐块一一对应映射）
-- 替换时检查 subtile 有效性：源地形块有效而目标地形块无效时，替换为默认地形
-- 预设中的三个数值均可为逗号分隔的列表，支持一组内多个范围，例如：
--   "100,300", "105,303", "400,500"
--   表示 100~105 → 400~405，300~303 → 500~503（目标区间长度自动与源区间一致）

-- 预设表：{ "显示名称", "起始1列表", "结束1列表", "起始2列表" }
-- 含义：把每组 [起始1, 结束1] 区间内的地形索引替换成 [起始2, 起始2 + (结束1-起始1)]，勾选反转时互换
local PRESETS = {
    TEMPERATE = {
		{"悬崖 → 水上悬崖", "49", "70", "148"},
    },
    SNOW = {
        {"悬崖 → 水上悬崖", "49", "70", "148"},
        {"冰川 → 水上冰川", "798", "819", "898"},
        {"悬崖 → 冰川", "49,384", "88,393", "798,878"},
        {"去除冰川补角（必须反转方向！）", "0,0,0,0", "0,0,0,0", "960,961,962,963"},
    },
    URBAN = {
		{"城市高台 → 水上高台", "49,796,797,798,799,800,801,802,803,804,805,806,807,808,809,901,1189", "70,796,797,798,799,800,801,802,803,804,805,806,807,808,809,902,1194", "148,837,836,835,834,833,832,831,830,829,828,827,826,825,824,903,1201"},
    },
    NEWURBAN = {
        {"自然悬崖 → 水上自然悬崖", "969", "990", "1049"},
		{"城市高台 → 水上高台", "49,796,797,798,799,800,801,802,803,804,805,806,807,808,809,888,1706", "70,796,797,798,799,800,801,802,803,804,805,806,807,808,809,889,1711", "148,837,836,835,834,833,832,831,830,829,828,827,826,825,824,890,1718"},
    },
    LUNAR = {
        
    },
    DESERT = {
        {"*TX*悬崖 → 水上悬崖", "49", "70", "727"},
    },
}

-- 解析逗号分隔的数字列表（支持数字或字符串输入），返回整数数组
local function split_nums(v)
    local list = {}
    if v == nil then return list end
    for item in string.gmatch(tostring(v), "[^,%s]+") do
        local n = tonumber(item)
        if n then
            list[#list + 1] = math.floor(n)
        end
    end
    return list
end

local th = theater()
local presets = PRESETS[th]
if not presets then
    message_box("当前地图类型 \"" .. tostring(th) .. "\" 没有对应的预设表！", "错误", 9)
    end_script()
end

local has_select = false
for x = 0, iso_size() - 1 do
    for y = 0, iso_size() - 1 do
        local cell = get_cell(x, y)
        if cell and cell:is_multi_selected() then
            has_select = true
            break
        end
    end
    if has_select then break end
end
if not has_select then
    message_box("请先用多选工具选中要替换的单元格！", "提示", 8)
    end_script()
end

local dlg = LuaDialog:new("地形批量替换", true)

local names = {}
for i, p in ipairs(presets) do
    names[#names + 1] = p[1]
end
names[#names + 1] = "自定义"

dlg:add_combobox("preset", "替换预设", names, names[1], true)
dlg:add_edit("n1", "起始1（源区间，逗号分隔）", "")
dlg:add_edit("n2", "结束1（源区间，逗号分隔）", "")
dlg:add_edit("n3", "起始2（目标区间，逗号分隔）", "")
dlg:add_checkbox("reverse", "反转方向（目标区间 → 源区间）", false)

-- 选中预设时填入数值并禁用输入框；选中"自定义"时启用输入框
dlg:on_event("preset", "selchange", function(key)
    local v = dlg:get_string("preset")
    if v == "自定义" then
        for _, k in ipairs({"n1", "n2", "n3"}) do
            dlg:set_enabled(k, true)
        end
    else
        for i, p in ipairs(presets) do
            if p[1] == v then
                dlg:set_text("n1", tostring(p[2]))
                dlg:set_text("n2", tostring(p[3]))
                dlg:set_text("n3", tostring(p[4]))
            end
        end
        for _, k in ipairs({"n1", "n2", "n3"}) do
            dlg:set_enabled(k, false)
        end
    end
end)

local result = dlg:do_modal()
if not result then
    end_script()
end

-- 解析三组逗号分隔的数字列表
local s1 = split_nums(result.n1)
local e1 = split_nums(result.n2)
local s2 = split_nums(result.n3)

if #s1 == 0 or #s1 ~= #e1 or #s1 ~= #s2 then
    message_box("请为起始1、结束1、起始2 输入数量相同的数字列表（用逗号分隔）！", "错误", 9)
    end_script()
end

local max_tile = tile_count()

-- 校验并构建映射组 {起始1, 结束1, 起始2, 结束2}
local groups = {}
for i = 1, #s1 do
    local a1, a2, b1 = s1[i], e1[i], s2[i]
    local b2 = b1 + (a2 - a1)   -- 目标区间长度与源区间一致
    if a1 < 0 or a2 < 0 or b1 < 0 or a1 > a2 then
        message_box("第 " .. i .. " 组的数字无效（均为非负整数，且起始1 ≤ 结束1）！", "错误", 9)
        end_script()
    end
    if a2 >= max_tile or b2 >= max_tile then
        message_box("第 " .. i .. " 组的区间超出当前地图的地形索引范围（0 ~ " .. tostring(max_tile - 1) .. "）！", "错误", 9)
        end_script()
    end
    groups[i] = { a1, a2, b1, b2 }
end

-- 反转方向：互换源区间与目标区间
if result.reverse == true then
    for i, g in ipairs(groups) do
        groups[i] = { g[3], g[4], g[1], g[2] }
    end
end

save_undo()

local total, modified, defaulted = 0, 0, 0
local default_coords = {}   -- 记录因目标子块无效而替换为默认地形的单元格坐标
print("正在遍历多选单元格，请稍候...")

for x = 0, iso_size() - 1 do
    for y = 0, iso_size() - 1 do
        local cell = get_cell(x, y)
        if cell and cell:is_multi_selected() then
            total = total + 1
            local t = cell.tile
            for gi = 1, #groups do
                local g = groups[gi]
                local from1, from2, to1 = g[1], g[2], g[3]
                if t >= from1 and t <= from2 then
                    local nt = to1 + (t - from1)
                    if nt >= 0 and nt < max_tile then
                        local st = cell.subtile
                        -- 源地形块有效而目标地形块无效时，替换为默认地形（tile=0, subtile=0）
                        local src_block = get_tile_block(t, st)
                        local dst_block = get_tile_block(nt, st)
                        if src_block and src_block.valid and not (dst_block and dst_block.valid) then
                            nt = 0
                            st = 0
                            defaulted = defaulted + 1
                            default_coords[#default_coords + 1] = string.format("(%d, %d)", x, y)
                        end
                        cell.tile = nt
                        cell.subtile = st
                        cell:apply()
                        modified = modified + 1
                    end
                    break
                end
            end
        end
    end
end

redraw_window()
update_minimap()

print(string.format("处理完成！共 %d 个多选单元格，替换了 %d 个地形块\n   （其中 %d 个因目标子块无效而替换为默认地形）", total, modified, defaulted))
if defaulted > 0 then
    print("替换为默认地形的单元格坐标：")
    for i, c in ipairs(default_coords) do
        print(c)
    end
end
