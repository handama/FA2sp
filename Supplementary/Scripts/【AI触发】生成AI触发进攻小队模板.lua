--【AI触发】生成进攻小队模板.lua

-- ========== 自动创建快照 ==========
create_snapshot()

-- ========== 初始化数据 ==========

-- 获取所有国家列表
local countries = {}
for i, house in pairs(get_values("Countries", "rules+map")) do
    table.insert(countries, {key = house, value = translate_house(house) .. " (" .. house .. ")"})
end
if is_multiplay() then
    for i = 0, 7 do
        local letter = string.char(string.byte("A") + i)
        table.insert(countries, {key = "<Player @ " .. letter .. ">", value = "<Player @ " .. letter .. ">"})
    end
end

-- 获取所有单位列表（步兵+车辆+飞行器）
local all_units = {}
-- 步兵
for k, v in pairs(get_values("InfantryTypes", "rules+map")) do
    if v ~= "" then
        table.insert(all_units, {id = v, name = get_uiname(v), cat = "步兵"})
    end
end
-- 车辆
for k, v in pairs(get_values("VehicleTypes", "rules+map")) do
    if v ~= "" then
        table.insert(all_units, {id = v, name = get_uiname(v), cat = "车辆"})
    end
end
-- 飞行器
for k, v in pairs(get_values("AircraftTypes", "rules+map")) do
    if v ~= "" then
        table.insert(all_units, {id = v, name = get_uiname(v), cat = "飞行器"})
    end
end
-- 排序：按类别+名称
table.sort(all_units, function(a, b)
    if a.cat ~= b.cat then return a.cat < b.cat end
    return a.name < b.name
end)

-- 构建Combobox显示列表："[步兵] 美国大兵 (E1)"
local unit_items = {}
for _, u in ipairs(all_units) do
    table.insert(unit_items, "[" .. u.cat .. "] " .. u.name .. " (" .. u.id .. ")")
end

-- ========== 创建对话框 ==========

local dlg = LuaDialog:new("进攻小队生成器", false, 580, 350)

-- 所属方选择
dlg:add_label("lbl_house", "所属方", 10, 27, 60, 20)
local country_items = {}
for _, c in ipairs(countries) do
    table.insert(country_items, c.value)
end
dlg:add_combobox("house", "", country_items, country_items[1] or "", true, 70, 10, 490, 22)

-- 列标题
dlg:add_label("col_idx", "#", 15, 62, 25, 16)
dlg:add_label("col_num", "数量", 40, 62, 40, 16)
dlg:add_label("col_unit", "单位类型", 90, 62, 460, 16)

-- 6个单位输入行
local unit_rows = {}
for i = 1, 6 do
    local y = 82 + (i - 1) * 32
    dlg:add_label("idx_" .. i, tostring(i) .. ".", 15, y + 13, 25, 22)
    dlg:add_edit("num_" .. i, "", "", 40, y - 5, 45, 22)
    dlg:add_combobox("unit_" .. i, "", unit_items, "", false, 90, y - 5, 460, 22)
    unit_rows[i] = {num_key = "num_" .. i, unit_key = "unit_" .. i}
end

-- ========== 显示对话框 ==========

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- ========== 解析输入 ==========

-- 解析所属方
local selected_value = result.house
local selected_house = ""
for _, c in ipairs(countries) do
    if c.value == selected_value then
        selected_house = c.key
        break
    end
end
if selected_house == "" then
    print("未选择有效的所属方，脚本已中止")
    return
end

-- 解析单位输入
local parsed_units = {}
for i = 1, 6 do
    local num_str = result["num_" .. i] or ""
    local unit_str = result["unit_" .. i] or ""

    -- 去除空格
    num_str = num_str:match("^%s*(.-)%s*$") or ""
    unit_str = unit_str:match("^%s*(.-)%s*$") or ""

    -- 跳过空行
    if num_str ~= "" and unit_str ~= "" then
        local num = tonumber(num_str)
        if num and num > 0 then
            -- 从 "[步兵] 美国大兵 (E1)" 中提取 "E1"
            local id = unit_str:match("%((.+)%)$")
            if id then
                table.insert(parsed_units, {number = num, id = id, display = unit_str})
                print(string.format("  %d) 数量: %d | 单位: %s", i, num, id))
            end
        end
    end
end

if #parsed_units == 0 then
    print("未输入任何有效的特遣部队，脚本已中止")
    return
end

-- ========== 计算所属阵营 ==========

local side_str = get_string(selected_house, "Side", "0", "rules+map")
local side_map = {GDI = 1, Nod = 2, ThirdSide = 3}
local side = side_map[side_str] or 0

-- ========== 创建特遣部队、脚本、小队、AI触发 ==========

-- 生成名称（去除特殊字符）
local name_parts = {}
for _, pu in ipairs(parsed_units) do
    table.insert(name_parts, pu.number .. pu.id)
end
local name_suffix = table.concat(name_parts, "")
local base_name = selected_house .. "_" .. name_suffix

-- 创建特遣部队
local task = task_force:new()
task.name = "<" .. base_name .. " ATK>"
for _, pu in ipairs(parsed_units) do
    task:add_number(pu.number, pu.id)
end
task:apply()

-- 创建脚本（0-0：攻击移动）
local s = script:new()
s.name = "<" .. base_name .. " ATK>"
s:add_action(0, 0)
s:apply()

-- 创建小队
local t = team:new()
t.name = "<" .. base_name .. " ATK>"
t.country = selected_house
t.task_force = task.id
t.script = s.id
t.aggressive = true
t.recruiter = true
t:apply()

-- 创建AI触发（进攻型，与防御型的区别）
local ai = ai_trigger:new()
ai.name = "<" .. base_name .. " ATK>"
ai.team1 = t.id
ai.country = selected_house
ai.tech_level = "1"
ai.condition = "-1"
ai.object = "<none>"
ai.comparator = "<"
ai.amount = 0
ai.side = tostring(side)
ai.init_weight = 50
ai.min_weight = 30
ai.max_weight = 50
ai.is_for_skirmish = false
ai.is_base_defense = false
ai.easy = true
ai.medium = true
ai.hard = true
ai.enabled = true
ai:apply()

-- ========== 输出结果 ==========

print("=======================")
print("进攻小队已生成：")
print("  特遣部队: " .. task.name .. " (ID: " .. task.id .. ")")
print("  脚本: " .. s.name .. " (ID: " .. s.id .. ")")
print("  小队: " .. t.name .. " (ID: " .. t.id .. ")")
print("  AI触发: " .. ai.name .. " (ID: " .. ai.id .. ")")
print("-----------------------")
print("小队属性：")
print("  √ 侵略部队")
print("  √ 忽视分组")
print("脚本行为：0-0（攻击移动）")
print("AI触发：已启用，阵营=" .. tostring(side))
print("=======================")

message_box("进攻小队已成功生成！\n\n特遣部队: " .. task.name .. "\n脚本: " .. s.name .. "\n小队: " .. t.name .. "\nAI触发: " .. ai.name, "执行成功", 1)