-- 生成AI触发模拟器的建立小队刷兵模板.lua

-- ========== 自动创建快照 ==========

create_snapshot()

-- ========== 获取数据 ==========

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

local all_units = {}
for k, v in pairs(get_values("InfantryTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "步兵"}) end
end
for k, v in pairs(get_values("VehicleTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "车辆"}) end
end
for k, v in pairs(get_values("AircraftTypes", "rules+map")) do
    if v ~= "" then table.insert(all_units, {id = v, name = get_uiname(v), cat = "飞行器"}) end
end
table.sort(all_units, function(a, b)
    if a.cat ~= b.cat then return a.cat < b.cat end
    return a.name < b.name
end)
local unit_items = {}
for _, u in ipairs(all_units) do
    table.insert(unit_items, "[" .. u.cat .. "] " .. u.name .. " (" .. u.id .. ")")
end

-- 获取已有局部变量
local var_items = {}
local var_keys = get_keys("VariableNames", "map")
for _, key in ipairs(var_keys) do
    local val = get_string("VariableNames", key, "", "map")
    local vname = val:match("^([^,]+)")
    table.insert(var_items, {index = tonumber(key), display = "[" .. key .. "] " .. (vname or "")})
end
table.sort(var_items, function(a, b) return a.index < b.index end)

local var_display_items = {}
for _, v in ipairs(var_items) do
    table.insert(var_display_items, v.display)
end

-- ========== 对话框 ==========

local dlg = LuaDialog:new("AI模拟器刷兵生成器", false, 580, 350)
local y = 10

-- 所属方
dlg:add_label("lbl_house", "所属方", 10, y + 18, 50, 20)
local country_items = {}
for _, c in ipairs(countries) do table.insert(country_items, c.value) end
dlg:add_combobox("house", "", country_items, country_items[1] or "", true, 65, y, 490, 22)

y = y + 28
dlg:add_label("col_idx", "#", 15, y + 18, 20, 16)
dlg:add_label("col_num", "数量", 40, y + 18, 40, 16)
dlg:add_label("col_unit", "单位类型（支持搜索）", 90, y + 18, 460, 16)

for i = 1, 6 do
    y = y + 24
    dlg:add_label("idx_" .. i, tostring(i) .. ".", 15, y + 18, 20, 22)
    dlg:add_edit("num_" .. i, "", "", 40, y, 40, 22)
    dlg:add_combobox("unit_" .. i, "", unit_items, "", false, 90, y, 460, 22)
end

y = y + 32
dlg:add_label("lbl_var", "AI模拟器变量", 10, y + 18, 90, 20)
if #var_display_items > 0 then
    dlg:add_combobox("variable", "", var_display_items, var_display_items[1], true, 100, y, 200, 22)
else
    dlg:add_label("lbl_no_var", "⚠ 暂无变量", 100, y + 18, 200, 20)
end

dlg:add_label("lbl_repeat", "重复类型", 320, y + 18, 60, 22)
local repeat_items = {"0 - 任一满足，单次", "1 - 所有满足，单次", "2 - 任一满足，重复"}
dlg:add_combobox("repeat_type", "", repeat_items, repeat_items[1], true, 380, y, 170, 22)

y = y + 28
dlg:add_label("lbl_interval", "计时器间隔(秒)", 10, y + 18, 110, 22)
dlg:add_edit("interval", "", "220", 120, y, 60, 22)

local result = dlg:do_modal()
if not result then
    print("用户取消了操作，脚本已中止")
    return
end

-- ========== 解析输入 ==========

local selected_house = ""
for _, c in ipairs(countries) do
    if c.value == result.house then selected_house = c.key; break end
end
if selected_house == "" then print("未选择有效的所属方"); return end

local parsed_units = {}
for i = 1, 6 do
    local ns = (result["num_" .. i] or ""):match("^%s*(.-)%s*$") or ""
    local us = (result["unit_" .. i] or ""):match("^%s*(.-)%s*$") or ""
    if ns ~= "" and us ~= "" then
        local n = tonumber(ns)
        local id = us:match("%((.+)%)$")
        if n and n > 0 and id then
            table.insert(parsed_units, {number = n, id = id})
        end
    end
end
if #parsed_units == 0 then print("未输入有效的特遣部队"); return end

local variable_index = nil
local var_display = result.variable or ""
for _, v in ipairs(var_items) do
    if v.display == var_display then variable_index = v.index; break end
end
if not variable_index then print("未选择有效的变量"); return end

local repeat_type_str = result.repeat_type or ""
local repeat_value = 0
if repeat_type_str:find("1") then repeat_value = 1
elseif repeat_type_str:find("2") then repeat_value = 2 end

local interval = tonumber((result.interval or ""):match("^%s*(.-)%s*$") or "")
if not interval or interval < 0 then interval = 220 end

-- ========== 创建特遣部队、脚本、小队 ==========

local name_parts = {}
for _, pu in ipairs(parsed_units) do table.insert(name_parts, pu.number .. pu.id) end
local base_name = selected_house .. "-" .. table.concat(name_parts, "")

local task = task_force:new(); task.name = base_name
for _, pu in ipairs(parsed_units) do task:add_number(pu.number, pu.id) end; task:apply()

local s = script:new(); s.name = base_name; s:add_action(0, 0); s:apply()

local t = team:new()
t.name = base_name; t.country = selected_house
t.task_force = task.id; t.script = s.id; t.recruiter = true; t:apply()

-- ========== 创建两个触发（两阶段：先建后补 Action）==========

local timer_suffix = (repeat_value == 2) and "重复计时器" or "单次计时器"
local spawn_suffix = (repeat_value == 2) and "重复刷兵" or "单次刷兵"

-- 先创建 trig2（刷兵触发），因为 trig1 需要引用它的 ID
local trig2 = trigger:new()
trig2.name = "[刷兵]" .. base_name .. "_" .. spawn_suffix
trig2.country = "Neutral"; trig2.easy = true; trig2.medium = true; trig2.hard = true; trig2.disabled = false
trig2:add_event("13,0,0")
trig2:add_event("36,0," .. variable_index)  -- 36: 变量==0
trig2:add_action("4,1," .. t.id .. ",0,0,0,0,A")
trig2:add_tag("", trig2.name .. " 1", repeat_value)
trig2:apply()
-- 此时 trig2.id 已确定，补上自禁用 Action
trig2:add_action("54,2," .. trig2.id .. ",0,0,0,0,A")
trig2:apply()

-- 再创建 trig1（计时器触发），引用已知的 trig2.id
local trig1 = trigger:new()
trig1.name = "[刷兵]" .. base_name .. "_" .. timer_suffix
trig1.country = "Neutral"; trig1.easy = true; trig1.medium = true; trig1.hard = true; trig1.disabled = false
trig1:add_event("13,0," .. interval)
trig1:add_action("53,2," .. trig2.id .. ",0,0,0,0,A")
trig1:add_tag("", trig1.name .. " 1", repeat_value)
trig1:apply()

-- ========== 输出结果 ==========

print("=======================")
print("AI模拟器刷兵已生成：")
print("  特遣部队: " .. task.name .. " (ID: " .. task.id .. ")")
print("  脚本: " .. s.name .. " (ID: " .. s.id .. ")")
print("  小队: " .. t.name .. " (ID: " .. t.id .. ")")
print("  计时器触发: " .. trig1.name .. " (ID: " .. trig1.id .. ")")
print("  刷兵触发: " .. trig2.name .. " (ID: " .. trig2.id .. ")")
print("  关联变量: [" .. variable_index .. "]")
print("-----------------------")
print("工作流程：")
print(string.format("  计时器 (每 %d 秒) → 启用刷兵触发", interval))
print("  刷兵触发 (变量=0时) → 建立小队 + 自禁用")
print("  只需开关刷兵触发的启用状态即可控制刷兵")
print("  计时器触发保持启用状态")
print("=======================")

message_box(string.format("AI模拟器刷兵已成功生成！\n计时器: %s\n刷兵: %s\n关联变量: [%d]\n\n计时器保持启用，\n开关刷兵触发来控制刷兵",
    trig1.name, trig2.name, variable_index), "执行成功", 1)